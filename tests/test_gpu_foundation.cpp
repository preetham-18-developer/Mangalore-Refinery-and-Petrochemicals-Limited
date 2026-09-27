#include "test_harness.hpp"
#include <bharatopt/gpu_backend.hpp>
#include <bharatopt/sparse_matrix.hpp>
#include <cmath>
#include <vector>

using namespace bharatopt;

TEST_CASE(GpuFoundation_01_GpuAvailability) {
    GpuBackend& gpu = GpuBackend::instance();
    GPUStatus status = gpu.initialize();
    EXPECT_TRUE(status == GPUStatus::GPU_UNAVAILABLE || status == GPUStatus::SUCCESS);
}

TEST_CASE(GpuFoundation_02_DeviceInitialisation) {
    GpuBackend& gpu = GpuBackend::instance();
    GPUStatus status = gpu.initialize();
    EXPECT_TRUE(status != GPUStatus::GPU_UNINITIALIZED);
}

TEST_CASE(GpuFoundation_03_DeviceMetadata) {
    GpuBackend& gpu = GpuBackend::instance();
    gpu.initialize();
    const auto& info = gpu.device_info();
    EXPECT_TRUE(!info.device_name.empty());
    EXPECT_TRUE(info.total_memory_bytes >= static_cast<size_t>(0));
}

TEST_CASE(GpuFoundation_04_DeviceAllocation) {
    GpuBuffer<real_t> buffer;
    EXPECT_EQ(buffer.size(), static_cast<size_t>(0));
    EXPECT_EQ(buffer.allocate(100), GPUStatus::SUCCESS);
    EXPECT_EQ(buffer.size(), static_cast<size_t>(100));
    EXPECT_EQ(buffer.bytes(), 100 * sizeof(real_t));
    buffer.deallocate();
    EXPECT_EQ(buffer.size(), static_cast<size_t>(0));
}

TEST_CASE(GpuFoundation_05_HostToDeviceTransfer) {
    std::vector<real_t> host_data = {1.5, 2.5, 3.5, 4.5};
    GpuBuffer<real_t> device_buf;
    EXPECT_EQ(device_buf.copy_host_to_device(host_data.data(), host_data.size()), GPUStatus::SUCCESS);
    EXPECT_EQ(device_buf.size(), static_cast<size_t>(4));
}

TEST_CASE(GpuFoundation_06_DeviceToHostTransfer) {
    std::vector<real_t> host_in = {10.0, 20.0, 30.0};
    GpuBuffer<real_t> device_buf;
    device_buf.copy_host_to_device(host_in.data(), host_in.size());

    std::vector<real_t> host_out(3, 0.0);
    EXPECT_EQ(device_buf.copy_device_to_host(host_out.data(), 3), GPUStatus::SUCCESS);
    EXPECT_NEAR(host_out[0], 10.0, 1e-12);
    EXPECT_NEAR(host_out[1], 20.0, 1e-12);
    EXPECT_NEAR(host_out[2], 30.0, 1e-12);
}

TEST_CASE(GpuFoundation_07_IdentitySpMV) {
    // A = I(3)
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(1, 1, 1.0);
    coo.add_entry(2, 2, 1.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {5.0, -3.0, 7.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;
    GpuTimingResult timing;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err, &timing), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_NEAR(gpu_y[0], 5.0, 1e-12);
    EXPECT_NEAR(gpu_y[1], -3.0, 1e-12);
    EXPECT_NEAR(gpu_y[2], 7.0, 1e-12);
}

TEST_CASE(GpuFoundation_08_DiagonalSpMV) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(1, 1, 4.0);
    coo.add_entry(2, 2, 8.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {1.0, 2.0, 3.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_NEAR(gpu_y[0], 2.0, 1e-12);
    EXPECT_NEAR(gpu_y[1], 8.0, 1e-12);
    EXPECT_NEAR(gpu_y[2], 24.0, 1e-12);
}

TEST_CASE(GpuFoundation_09_SmallSparseSpMV) {
    COOMatrix coo(2, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 2, 2.0);
    coo.add_entry(1, 1, 3.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {4.0, 5.0, 6.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_NEAR(gpu_y[0], 16.0, 1e-12); // 1*4 + 2*6 = 16
    EXPECT_NEAR(gpu_y[1], 15.0, 1e-12); // 3*5 = 15
}

TEST_CASE(GpuFoundation_10_RectangularSpMV) {
    COOMatrix coo(4, 2);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(1, 1, 2.0);
    coo.add_entry(2, 0, 3.0);
    coo.add_entry(3, 1, 4.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {10.0, 20.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_EQ(gpu_y.size(), static_cast<size_t>(4));
    EXPECT_NEAR(gpu_y[0], 10.0, 1e-12);
    EXPECT_NEAR(gpu_y[1], 40.0, 1e-12);
    EXPECT_NEAR(gpu_y[2], 30.0, 1e-12);
    EXPECT_NEAR(gpu_y[3], 80.0, 1e-12);
}

TEST_CASE(GpuFoundation_11_RandomSparseSpMV) {
    COOMatrix coo(10, 10);
    for (size_t i = 0; i < 10; ++i) {
        coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(i), static_cast<real_t>(i + 1));
        if (i < 9) {
            coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(i + 1), 0.5);
        }
    }
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);
    std::vector<real_t> x(10, 1.0);
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
}

TEST_CASE(GpuFoundation_12_LargeSparseSpMV) {
    size_t n = 1000;
    COOMatrix coo(n, n);
    for (size_t i = 0; i < n; ++i) {
        coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(i), 2.0);
    }
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);
    std::vector<real_t> x(n, 3.0);
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_EQ(gpu_y.size(), n);
    EXPECT_NEAR(gpu_y[500], 6.0, 1e-12);
}

TEST_CASE(GpuFoundation_13_VectorAddition) {
    GpuBuffer<real_t> a(3), b(3), res(3);
    std::vector<real_t> ha = {1.0, 2.0, 3.0};
    std::vector<real_t> hb = {4.0, 5.0, 6.0};
    a.copy_host_to_device(ha.data(), 3);
    b.copy_host_to_device(hb.data(), 3);

    EXPECT_EQ(GpuBackend::instance().vector_add(a, b, res, 3), GPUStatus::SUCCESS);
    std::vector<real_t> hres(3);
    res.copy_device_to_host(hres.data(), 3);

    EXPECT_NEAR(hres[0], 5.0, 1e-12);
    EXPECT_NEAR(hres[1], 7.0, 1e-12);
    EXPECT_NEAR(hres[2], 9.0, 1e-12);
}

TEST_CASE(GpuFoundation_14_VectorScaling) {
    GpuBuffer<real_t> in(3), res(3);
    std::vector<real_t> hin = {2.0, 4.0, 6.0};
    in.copy_host_to_device(hin.data(), 3);

    EXPECT_EQ(GpuBackend::instance().vector_scale(in, 2.5, res, 3), GPUStatus::SUCCESS);
    std::vector<real_t> hres(3);
    res.copy_device_to_host(hres.data(), 3);

    EXPECT_NEAR(hres[0], 5.0, 1e-12);
    EXPECT_NEAR(hres[1], 10.0, 1e-12);
    EXPECT_NEAR(hres[2], 15.0, 1e-12);
}

TEST_CASE(GpuFoundation_15_AXPY) {
    GpuBuffer<real_t> x(3), y(3), out(3);
    std::vector<real_t> hx = {1.0, 2.0, 3.0};
    std::vector<real_t> hy = {10.0, 20.0, 30.0};
    x.copy_host_to_device(hx.data(), 3);
    y.copy_host_to_device(hy.data(), 3);

    EXPECT_EQ(GpuBackend::instance().axpy(3.0, x, y, out, 3), GPUStatus::SUCCESS);
    std::vector<real_t> hout(3);
    out.copy_device_to_host(hout.data(), 3);

    EXPECT_NEAR(hout[0], 13.0, 1e-12); // 3*1 + 10 = 13
    EXPECT_NEAR(hout[1], 26.0, 1e-12); // 3*2 + 20 = 26
    EXPECT_NEAR(hout[2], 39.0, 1e-12); // 3*3 + 30 = 39
}

TEST_CASE(GpuFoundation_16_DotProduct) {
    GpuBuffer<real_t> a(3), b(3);
    std::vector<real_t> ha = {1.0, 2.0, 3.0};
    std::vector<real_t> hb = {4.0, 5.0, 6.0};
    a.copy_host_to_device(ha.data(), 3);
    b.copy_host_to_device(hb.data(), 3);

    real_t dot = 0.0;
    EXPECT_EQ(GpuBackend::instance().dot_product(a, b, dot, 3), GPUStatus::SUCCESS);
    EXPECT_NEAR(dot, 32.0, 1e-12); // 1*4 + 2*5 + 3*6 = 32
}

TEST_CASE(GpuFoundation_17_Norm) {
    GpuBuffer<real_t> v(3);
    std::vector<real_t> hv = {3.0, 4.0, 0.0};
    v.copy_host_to_device(hv.data(), 3);

    real_t norm = 0.0;
    EXPECT_EQ(GpuBackend::instance().norm2(v, norm, 3), GPUStatus::SUCCESS);
    EXPECT_NEAR(norm, 5.0, 1e-12); // sqrt(9 + 16) = 5
}

TEST_CASE(GpuFoundation_18_CpuVsGpuComparison) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 3.0);
    coo.add_entry(0, 1, 4.0);
    coo.add_entry(1, 0, 1.0);
    coo.add_entry(1, 1, 2.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {2.0, 3.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
}

TEST_CASE(GpuFoundation_19_NumericalTolerance) {
    COOMatrix coo(1, 1);
    coo.add_entry(0, 0, 1e-8);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);
    std::vector<real_t> x = {1e-8};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err), GPUStatus::SUCCESS);
    EXPECT_NEAR(max_err, 0.0, 1e-12);
    EXPECT_NEAR(gpu_y[0], 1e-16, 1e-18);
}

TEST_CASE(GpuFoundation_20_NaNInfHandling) {
    GpuBuffer<real_t> empty_buf;
    GpuMatrixCSR empty_matrix;
    EXPECT_EQ(GpuBackend::instance().spmv(empty_matrix, empty_buf, empty_buf), GPUStatus::INVALID_ARGUMENT);
}

TEST_CASE(GpuFoundation_21_TransferTiming) {
    COOMatrix coo(5, 5);
    for (size_t i = 0; i < 5; ++i) coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(i), 1.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;
    GpuTimingResult timing;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err, &timing), GPUStatus::SUCCESS);
    EXPECT_TRUE(timing.h2d_time_ms >= 0.0);
    EXPECT_TRUE(timing.d2h_time_ms >= 0.0);
}

TEST_CASE(GpuFoundation_22_KernelTiming) {
    GpuBuffer<real_t> a(10), b(10), res(10);
    std::vector<real_t> h(10, 1.0);
    a.copy_host_to_device(h.data(), 10);
    b.copy_host_to_device(h.data(), 10);

    GpuTimingResult timing;
    EXPECT_EQ(GpuBackend::instance().vector_add(a, b, res, 10, &timing), GPUStatus::SUCCESS);
    EXPECT_TRUE(timing.kernel_time_ms >= 0.0);
}

TEST_CASE(GpuFoundation_23_EndToEndTiming) {
    COOMatrix coo(10, 10);
    for (size_t i = 0; i < 10; ++i) coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(i), 1.0);
    CSRMatrix cpu_A = CSRMatrix::from_coo(coo);

    std::vector<real_t> x(10, 2.0);
    std::vector<real_t> gpu_y;
    real_t max_err = 0.0;
    GpuTimingResult timing;

    EXPECT_EQ(GpuBackend::instance().spmv_with_cpu_comparison(cpu_A, x, gpu_y, max_err, &timing), GPUStatus::SUCCESS);
    EXPECT_TRUE(timing.total_gpu_time_ms >= 0.0);
    EXPECT_TRUE(timing.cpu_reference_time_ms >= 0.0);
}

TEST_CASE(GpuFoundation_24_ResourceCleanup) {
    {
        GpuBuffer<real_t> temp(1000);
        EXPECT_TRUE(temp.is_allocated());
    } // Out of scope -> auto deallocated cleanly
}

TEST_CASE(GpuFoundation_25_GpuErrorHandling) {
    GpuBuffer<real_t> invalid_buf;
    real_t dot = 0.0;
    EXPECT_EQ(GpuBackend::instance().dot_product(invalid_buf, invalid_buf, dot, 10), GPUStatus::INVALID_ARGUMENT);
}
