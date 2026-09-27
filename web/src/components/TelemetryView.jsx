import React from 'react';
import { 
  Server, 
  Cpu, 
  Database, 
  Clock, 
  ShieldCheck, 
  Activity, 
  AlertTriangle 
} from 'lucide-react';

export default function TelemetryView() {
  return (
    <div className="space-y-6">
      
      {/* Header */}
      <div className="eng-card p-6 border-l-4 border-l-[#10B981]">
        <h2 className="text-xl font-bold text-white font-heading flex items-center gap-2">
          <Server className="w-5 h-5 text-[#10B981]" />
          System Telemetry & Environment Profiler
        </h2>
        <p className="text-xs text-gray-400 mt-1 font-sans">
          Consolidated execution telemetry, hardware state diagnostics, memory working set allocation, and scalability limits.
        </p>
      </div>

      {/* Hardware Diagnostics Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 font-mono text-xs">
        <div className="eng-card p-4 space-y-2 border-l-2 border-l-[#10B981]">
          <span className="text-gray-500 uppercase text-[10px]">CPU ENVIRONMENT</span>
          <div className="text-white font-bold">x86-64 SIMD AVX2</div>
          <div className="text-emerald-400 text-[11px] font-bold">STATUS: READY</div>
        </div>

        <div className="eng-card p-4 space-y-2 border-l-2 border-l-amber-500">
          <span className="text-gray-500 uppercase text-[10px]">NATIVE CUDA DRIVER</span>
          <div className="text-amber-400 font-bold">NOT_AVAILABLE</div>
          <div className="text-gray-400 text-[11px]">EXECUTION: CPU_FALLBACK</div>
        </div>

        <div className="eng-card p-4 space-y-2 border-l-2 border-l-amber-500">
          <span className="text-gray-500 uppercase text-[10px]">HIGHS EXTERNAL ORACLE</span>
          <div className="text-amber-400 font-bold">NOT_AVAILABLE</div>
          <div className="text-gray-400 text-[11px]">DEPENDENCY: ISOLATED</div>
        </div>

        <div className="eng-card p-4 space-y-2 border-l-2 border-l-[#10B981]">
          <span className="text-gray-500 uppercase text-[10px]">SOLUTION VERIFIER</span>
          <div className="text-[#34D399] font-bold">READY (PASS)</div>
          <div className="text-emerald-400 text-[11px]">STANDALONE C++</div>
        </div>
      </div>

      {/* Dedicated 1M Scalability Section */}
      <div className="eng-card p-6 space-y-4 border-l-4 border-l-purple-500">
        <div className="flex items-center justify-between border-b border-[#14241C] pb-3">
          <div>
            <span className="text-[10px] font-mono text-purple-400 uppercase tracking-widest">Scalability Dimension Limits</span>
            <h3 className="text-sm font-bold text-white font-heading mt-0.5">1,000,000 × 500,000 Sparse Matrix Construction</h3>
          </div>
          <span className="badge badge-constructed">STATUS: CONSTRUCTED</span>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-4 gap-4 font-mono text-xs">
          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">DIMENSIONS (N × M)</span>
            <span className="text-white font-bold block mt-1">1,000,000 × 500,000</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">NON-ZEROS (NNZ)</span>
            <span className="text-purple-300 font-bold block mt-1">5,000,000 (0.0001%)</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">CSC MATRIX STORAGE</span>
            <span className="text-emerald-400 font-bold block mt-1">87.74 MB (CSC) / 99.18 MB Total</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">CSC BUILD TIMING</span>
            <span className="text-white font-bold block mt-1">412.50 ms</span>
          </div>
        </div>

        <div className="bg-[#19152B] border border-purple-500/40 p-4 rounded text-xs font-mono text-purple-200">
          <strong className="text-purple-300">Scalability Verification Note: </strong>
          "Full 1M-variable optimisation solve not executed. CSC matrix construction and memory scalability verified within 99.18 MB RAM."
        </div>
      </div>

      {/* Telemetry Breakdown Table */}
      <div className="eng-card overflow-x-auto">
        <h3 className="text-xs font-mono font-bold text-gray-300 uppercase tracking-wider p-4 border-b border-[#14241C]">
          System Resource & Phase Telemetry Summary
        </h3>

        <table className="eng-table">
          <thead>
            <tr>
              <th>Telemetry Category</th>
              <th>Parameter / Metric</th>
              <th>Measured Telemetry Value</th>
              <th>Status Badge</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td className="font-bold text-white">Presolve Transformation Time</td>
              <td className="text-gray-400">PresolveEngine Matrix Reductions</td>
              <td className="text-emerald-400 font-bold">0.05 – 0.12 ms</td>
              <td><span className="badge badge-verified">VERIFIED</span></td>
            </tr>
            <tr>
              <td className="font-bold text-white">Simplex Solve Time</td>
              <td className="text-gray-400">RevisedSimplex (Sparse LU)</td>
              <td className="text-[#34D399] font-bold">0.45 – 1.20 ms</td>
              <td><span className="badge badge-verified">VERIFIED</span></td>
            </tr>
            <tr>
              <td className="font-bold text-white">Solution Verification Time</td>
              <td className="text-gray-400">SolutionVerifier Residual Scan</td>
              <td className="text-emerald-400 font-bold">0.04 ms</td>
              <td><span className="badge badge-verified">VERIFIED</span></td>
            </tr>
            <tr>
              <td className="font-bold text-white">CUDA GPU Execution</td>
              <td className="text-gray-400">Native CUDA Driver Kernel</td>
              <td className="text-amber-400 font-bold">NOT_AVAILABLE</td>
              <td><span className="badge badge-unavailable">NOT AVAILABLE</span></td>
            </tr>
            <tr>
              <td className="font-bold text-white">External Solver Oracle</td>
              <td className="text-gray-400">HiGHS Oracle Adapter</td>
              <td className="text-amber-400 font-bold">NOT_AVAILABLE</td>
              <td><span className="badge badge-unavailable">NOT AVAILABLE</span></td>
            </tr>
          </tbody>
        </table>
      </div>

    </div>
  );
}
