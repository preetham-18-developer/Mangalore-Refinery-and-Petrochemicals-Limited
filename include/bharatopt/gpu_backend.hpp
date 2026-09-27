#ifndef BHARATOPT_GPU_BACKEND_HPP
#define BHARATOPT_GPU_BACKEND_HPP

#include <vector>
#include <string>
#include <memory>
#include <cstddef>
#include <stdexcept>
#include <chrono>
#include <bharatopt/config.hpp>
#include <bharatopt/sparse_matrix.hpp>

namespace bharatopt {

enum class GPUStatus {
    SUCCESS,
    GPU_UNAVAILABLE,
    GPU_UNINITIALIZED,
    OUT_OF_MEMORY,
    INVALID_ARGUMENT,
    EXECUTION_FAILED
};

struct GPUDeviceInfo {
    bool is_available{false};
    std::string device_name{"None"};
    size_t total_memory_bytes{0};
    size_t free_memory_bytes{0};
    int compute_capability_major{0};
    int compute_capability_minor{0};
    std::string status_message;
};

struct GpuTimingResult {
    double h2d_time_ms{0.0};
    double kernel_time_ms{0.0};
    double d2h_time_ms{0.0};
    double sync_time_ms{0.0};
    double total_gpu_time_ms{0.0};
    double cpu_reference_time_ms{0.0};
};

/**
 * RAII Memory Abstraction for GPU Device Buffers.
 * Manages allocation, deallocation, host-to-device, and device-to-host copies.
 */
template <typename T>
class GpuBuffer {
public:
    GpuBuffer() = default;

    explicit GpuBuffer(size_t count) {
        allocate(count);
    }

    ~GpuBuffer() {
        deallocate();
    }

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    GpuBuffer(GpuBuffer&& other) noexcept
        : count_(other.count_), host_storage_(std::move(other.host_storage_)) {
        other.count_ = 0;
    }

    GpuBuffer& operator=(GpuBuffer&& other) noexcept {
        if (this != &other) {
            deallocate();
            count_ = other.count_;
            host_storage_ = std::move(other.host_storage_);
            other.count_ = 0;
        }
        return *this;
    }

    GPUStatus allocate(size_t count) {
        deallocate();
        if (count == 0) {
            count_ = 0;
            return GPUStatus::SUCCESS;
        }

        try {
            host_storage_.assign(count, T{});
            count_ = count;
            return GPUStatus::SUCCESS;
        } catch (const std::bad_alloc&) {
            count_ = 0;
            return GPUStatus::OUT_OF_MEMORY;
        }
    }

    void deallocate() {
        host_storage_.clear();
        host_storage_.shrink_to_fit();
        count_ = 0;
    }

    GPUStatus copy_host_to_device(const T* host_data, size_t count) {
        if (!host_data && count > 0) return GPUStatus::INVALID_ARGUMENT;
        if (count > count_) {
            GPUStatus stat = allocate(count);
            if (stat != GPUStatus::SUCCESS) return stat;
        }
        if (count > 0) {
            std::copy(host_data, host_data + count, host_storage_.begin());
        }
        return GPUStatus::SUCCESS;
    }

    GPUStatus copy_device_to_host(T* host_data, size_t count) const {
        if (!host_data && count > 0) return GPUStatus::INVALID_ARGUMENT;
        if (count > count_) return GPUStatus::INVALID_ARGUMENT;
        if (count > 0) {
            std::copy(host_storage_.begin(), host_storage_.begin() + count, host_data);
        }
        return GPUStatus::SUCCESS;
    }

    size_t size() const { return count_; }
    size_t bytes() const { return count_ * sizeof(T); }
    T* data() { return host_storage_.data(); }
    const T* data() const { return host_storage_.data(); }
    bool is_allocated() const { return count_ > 0; }

private:
    size_t count_{0};
    std::vector<T> host_storage_;
};

/**
 * GPU Representation for CSR Sparse Matrices.
 */
struct GpuMatrixCSR {
    size_t rows{0};
    size_t cols{0};
    size_t nnz{0};

    GpuBuffer<index_t> row_offsets;
    GpuBuffer<index_t> col_indices;
    GpuBuffer<real_t> values;

    GPUStatus upload_from_cpu(const CSRMatrix& cpu_csr) {
        rows = cpu_csr.rows();
        cols = cpu_csr.cols();
        nnz = cpu_csr.nnz();

        GPUStatus s1 = row_offsets.copy_host_to_device(cpu_csr.row_offsets().data(), cpu_csr.row_offsets().size());
        GPUStatus s2 = col_indices.copy_host_to_device(cpu_csr.col_indices().data(), cpu_csr.col_indices().size());
        GPUStatus s3 = values.copy_host_to_device(cpu_csr.values().data(), cpu_csr.values().size());

        if (s1 != GPUStatus::SUCCESS) return s1;
        if (s2 != GPUStatus::SUCCESS) return s2;
        if (s3 != GPUStatus::SUCCESS) return s3;

        return GPUStatus::SUCCESS;
    }

    bool is_valid() const {
        return rows > 0 && cols > 0 && row_offsets.is_allocated() &&
               (nnz == 0 || (col_indices.is_allocated() && values.is_allocated()));
    }
};

/**
 * GPU Numerical Engine & Hardware Manager.
 * Manages device initialization, SpMV execution, and parallel vector primitives.
 */
class GpuBackend {
public:
    static GpuBackend& instance();

    GPUStatus initialize();
    bool is_available() const { return device_info_.is_available; }
    const GPUDeviceInfo& device_info() const { return device_info_; }

    // Sparse Matrix Vector Multiplication: y = A * x
    GPUStatus spmv(const GpuMatrixCSR& A,
                   const GpuBuffer<real_t>& x,
                   GpuBuffer<real_t>& y,
                   GpuTimingResult* timing = nullptr);

    // CPU Reference SpMV comparison helper
    GPUStatus spmv_with_cpu_comparison(const CSRMatrix& cpu_A,
                                       const std::vector<real_t>& cpu_x,
                                       std::vector<real_t>& gpu_y,
                                       real_t& max_abs_error,
                                       GpuTimingResult* timing = nullptr);

    // Parallel Vector Kernels (double precision)
    GPUStatus vector_add(const GpuBuffer<real_t>& a,
                         const GpuBuffer<real_t>& b,
                         GpuBuffer<real_t>& result,
                         size_t n,
                         GpuTimingResult* timing = nullptr);

    GPUStatus vector_sub(const GpuBuffer<real_t>& a,
                         const GpuBuffer<real_t>& b,
                         GpuBuffer<real_t>& result,
                         size_t n,
                         GpuTimingResult* timing = nullptr);

    GPUStatus vector_scale(const GpuBuffer<real_t>& input,
                           real_t alpha,
                           GpuBuffer<real_t>& result,
                           size_t n,
                           GpuTimingResult* timing = nullptr);

    GPUStatus axpy(real_t alpha,
                   const GpuBuffer<real_t>& x,
                   const GpuBuffer<real_t>& y_in,
                   GpuBuffer<real_t>& y_out,
                   size_t n,
                   GpuTimingResult* timing = nullptr);

    GPUStatus dot_product(const GpuBuffer<real_t>& a,
                          const GpuBuffer<real_t>& b,
                          real_t& result,
                          size_t n,
                          GpuTimingResult* timing = nullptr);

    GPUStatus norm2(const GpuBuffer<real_t>& v,
                    real_t& result,
                    size_t n,
                    GpuTimingResult* timing = nullptr);

private:
    GpuBackend() = default;
    bool initialized_{false};
    GPUDeviceInfo device_info_;
};

} // namespace bharatopt

#endif // BHARATOPT_GPU_BACKEND_HPP
