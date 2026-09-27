import React from 'react';
import { 
  Cpu, 
  BarChart2, 
  ChevronRight, 
  CheckCircle2, 
  Server, 
  Zap, 
  Layers 
} from 'lucide-react';

export default function AdaptiveRouterView() {
  return (
    <div className="space-y-6">
      
      {/* Header */}
      <div className="eng-card p-6 border-l-4 border-l-[#10B981]">
        <h2 className="text-xl font-bold text-white font-heading flex items-center gap-2">
          <Cpu className="w-5 h-5 text-[#10B981]" />
          Adaptive Router & Crossover Profiler
        </h2>
        <p className="text-xs text-gray-400 mt-1 font-sans">
          Dynamic execution path selector analyzing problem dimensions, matrix density, condition numbers, and hardware capability to select optimal CPU or GPU solver backends.
        </p>
      </div>

      {/* Visual Technical Router Flowchart */}
      <div className="eng-card p-6 space-y-4">
        <h3 className="text-xs font-mono font-bold text-gray-300 uppercase tracking-wider border-b border-[#14241C] pb-3">
          Adaptive Routing Decision Pipeline
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-5 gap-3 text-center text-xs font-mono">
          <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Input</span>
            <strong className="text-white block">Problem Features</strong>
            <span className="text-[10px] text-gray-400 block">n, m, NNZ, Density</span>
          </div>

          <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Profiler</span>
            <strong className="text-emerald-400 block">Cost Estimator</strong>
            <span className="text-[10px] text-gray-400 block">Polynomial Model</span>
          </div>

          <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Decision</span>
            <strong className="text-amber-400 block">Hardware Target</strong>
            <span className="text-[10px] text-amber-300 block">CPU Fallback</span>
          </div>

          <div className="bg-[#0E1D17] p-4 rounded border border-[#10B981] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Selected Path</span>
            <strong className="text-[#34D399] block">DUAL REVISED SIMPLEX</strong>
            <span className="text-[10px] text-emerald-400 block">Sparse LU Engine</span>
          </div>

          <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Verification</span>
            <strong className="text-[#34D399] block">SolutionVerifier</strong>
            <span className="text-[10px] text-emerald-400 block">100% PASS</span>
          </div>
        </div>
      </div>

      {/* Feature Profiling & Predicted Cost Comparison Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        
        {/* Router Feature Inputs */}
        <div className="eng-card p-5 space-y-4">
          <h3 className="text-xs font-mono font-bold text-gray-300 uppercase tracking-wider border-b border-[#14241C] pb-3 flex items-center justify-between">
            <span>Problem Profiler Features</span>
            <span className="text-emerald-400 text-[10px]">Real Telemetry</span>
          </h3>

          <div className="space-y-2 font-mono text-xs">
            <div className="flex justify-between p-2 rounded bg-[#080E0B] border border-[#14241C]">
              <span className="text-gray-400">Problem Dimension (N × M):</span>
              <span className="text-white font-bold">32 × 27</span>
            </div>
            <div className="flex justify-between p-2 rounded bg-[#080E0B] border border-[#14241C]">
              <span className="text-gray-400">Non-Zeros (NNZ):</span>
              <span className="text-white font-bold">88</span>
            </div>
            <div className="flex justify-between p-2 rounded bg-[#080E0B] border border-[#14241C]">
              <span className="text-gray-400">Sparsity Density:</span>
              <span className="text-emerald-400 font-bold">10.18%</span>
            </div>
            <div className="flex justify-between p-2 rounded bg-[#080E0B] border border-[#14241C]">
              <span className="text-gray-400">Dynamic Coefficient Range:</span>
              <span className="text-white font-bold">10⁰ to 10⁴</span>
            </div>
            <div className="flex justify-between p-2 rounded bg-[#080E0B] border border-[#14241C]">
              <span className="text-gray-400">Presolve Reduction %:</span>
              <span className="text-[#34D399] font-bold">44.4% Rows Removed</span>
            </div>
          </div>
        </div>

        {/* Cost Model Prediction & Selection */}
        <div className="eng-card p-5 space-y-4">
          <h3 className="text-xs font-mono font-bold text-gray-300 uppercase tracking-wider border-b border-[#14241C] pb-3 flex items-center justify-between">
            <span>Predicted Cost & Rationale</span>
            <span className="text-amber-400 text-[10px]">Decision Matrix</span>
          </h3>

          <div className="space-y-3 font-mono text-xs">
            <div className="p-3 rounded bg-[#080E0B] border border-[#14241C] space-y-1">
              <div className="flex justify-between">
                <span className="text-gray-400">Predicted CPU Cost:</span>
                <span className="text-emerald-400 font-bold">0.45 ms</span>
              </div>
              <div className="flex justify-between">
                <span className="text-gray-400">Predicted GPU Cost:</span>
                <span className="text-amber-400 font-bold">12.80 ms (Transfer Overhead)</span>
              </div>
            </div>

            <div className="p-3 rounded bg-[#0E1D17] border border-[#10B981] space-y-1">
              <span className="text-gray-400 block text-[10px] uppercase">SELECTED ENGINE</span>
              <div className="text-base font-bold text-white">DUAL REVISED SIMPLEX (CPU)</div>
              <p className="text-[11px] text-gray-300 font-sans mt-1">
                Rationale: Problem size N = 32 &lt; 1000. CPU Revised Simplex avoids PCIe host-to-device memory transfer overheads and guarantees exact dual basis factorisation.
              </p>
            </div>
          </div>
        </div>

      </div>

    </div>
  );
}
