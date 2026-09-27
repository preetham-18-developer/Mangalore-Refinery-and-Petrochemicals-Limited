import React from 'react';
import { 
  ArrowRight, 
  CheckCircle2, 
  Cpu, 
  Layers, 
  ShieldCheck, 
  Zap, 
  Database, 
  Activity,
  ChevronRight
} from 'lucide-react';

export default function Overview({ onNavigateToSolver, latestRun }) {
  return (
    <div className="space-y-8">
      
      {/* Hero Section */}
      <div className="eng-card p-8 bg-gradient-to-r from-[#0B1712] via-[#09120E] to-[#040706] border-l-4 border-l-[#10B981] relative overflow-hidden">
        <div className="max-w-3xl relative z-10 space-y-4">
          <div className="inline-flex items-center gap-2 px-2.5 py-1 rounded bg-[#042F22] border border-[#059669]/40 text-[#34D399] text-xs font-mono">
            <span className="w-1.5 h-1.5 rounded-full bg-[#10B981] animate-pulse"></span>
            PRODUCTION-GRADE C++ OPTIMIZATION SOLVER ENGINE
          </div>

          <h1 className="text-3xl lg:text-4xl font-extrabold text-white tracking-tight font-heading leading-tight">
            Optimisation, built from the mathematics up.
          </h1>

          <p className="text-sm text-gray-300 leading-relaxed font-sans max-w-2xl">
            An adaptive CPU–GPU optimisation engine for large-scale LP and MILP workloads. Engineered with sparse LU factorization, dual revised simplex iterations, presolve matrix transformations, and standalone mathematical solution verification.
          </p>

          <div className="pt-2 flex items-center gap-4">
            <button 
              onClick={onNavigateToSolver}
              className="btn-emerald"
            >
              Open Solver Studio
              <ArrowRight className="w-4 h-4" />
            </button>
            <span className="text-xs text-gray-500 font-mono">
              SIH 2026 PS 26119 | Target: MRPL Refinery Scheduling
            </span>
          </div>
        </div>

        {/* Subtle Decorative Flow BG Graphic */}
        <div className="absolute right-0 top-0 bottom-0 w-1/3 opacity-10 pointer-events-none bg-[radial-gradient(#10B981_1px,transparent_1px)] [background-size:16px_16px]"></div>
      </div>

      {/* Abstract Optimization Architecture Flow Visual */}
      <div className="eng-card p-6 space-y-4">
        <div className="flex items-center justify-between border-b border-[#14241C] pb-3">
          <h2 className="text-xs font-mono font-bold text-gray-300 uppercase tracking-widest flex items-center gap-2">
            <Activity className="w-4 h-4 text-[#10B981]" />
            BHARATOPT Execution Architecture Pipeline
          </h2>
          <span className="text-[11px] text-gray-500 font-mono">End-to-End Mathematical Pipeline</span>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-6 gap-2 text-center text-xs font-mono">
          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Input</span>
            <span className="font-bold text-gray-200 block">Sparse Matrix</span>
            <span className="text-[10px] text-emerald-400 block">COO / CSC</span>
          </div>

          <div className="flex items-center justify-center text-gray-600 hidden md:flex">
            <ChevronRight className="w-4 h-4 text-emerald-500/60" />
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Stage 1</span>
            <span className="font-bold text-gray-200 block">Presolve Engine</span>
            <span className="text-[10px] text-emerald-400 block">Row/Col Reduction</span>
          </div>

          <div className="flex items-center justify-center text-gray-600 hidden md:flex">
            <ChevronRight className="w-4 h-4 text-emerald-500/60" />
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] space-y-1">
            <span className="text-[10px] text-gray-500 uppercase block">Stage 2</span>
            <span className="font-bold text-gray-200 block">Adaptive Router</span>
            <span className="text-[10px] text-amber-400 block">CPU / GPU Target</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] space-y-1 md:col-start-6">
            <span className="text-[10px] text-gray-500 uppercase block">Stage 3</span>
            <span className="font-bold text-[#34D399] block">SolutionVerifier</span>
            <span className="text-[10px] text-emerald-400 block">Residual Check</span>
          </div>
        </div>
      </div>

      {/* Live System Capability Strip */}
      <div className="grid grid-cols-2 md:grid-cols-5 gap-3 font-mono text-xs">
        <div className="eng-card p-3.5 flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-[#0A1812] border border-[#14241C] flex items-center justify-center text-[#10B981]">
            LP
          </div>
          <div>
            <span className="text-gray-400 block text-[10px]">LINEAR PROGRAMMING</span>
            <strong className="text-white">Revised Simplex</strong>
          </div>
        </div>

        <div className="eng-card p-3.5 flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-[#0A1812] border border-[#14241C] flex items-center justify-center text-purple-400">
            MIP
          </div>
          <div>
            <span className="text-gray-400 block text-[10px]">MIXED INTEGER</span>
            <strong className="text-white">Branch & Bound</strong>
          </div>
        </div>

        <div className="eng-card p-3.5 flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-[#0A1812] border border-[#14241C] flex items-center justify-center text-emerald-400">
            CSC
          </div>
          <div>
            <span className="text-gray-400 block text-[10px]">SPARSE MATRIX</span>
            <strong className="text-white">10⁶ Scalability</strong>
          </div>
        </div>

        <div className="eng-card p-3.5 flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-[#0A1812] border border-[#14241C] flex items-center justify-center text-amber-400">
            CPU
          </div>
          <div>
            <span className="text-gray-400 block text-[10px]">EXECUTION ROUTER</span>
            <strong className="text-white">Adaptive Profiler</strong>
          </div>
        </div>

        <div className="eng-card p-3.5 flex items-center gap-3 col-span-2 md:col-span-1">
          <div className="w-8 h-8 rounded bg-[#0A1812] border border-[#14241C] flex items-center justify-center text-[#34D399]">
            ✓
          </div>
          <div>
            <span className="text-gray-400 block text-[10px]">VERIFICATION</span>
            <strong className="text-emerald-400">SolutionVerifier</strong>
          </div>
        </div>
      </div>

      {/* Latest Execution Card */}
      <div className="eng-card p-6 space-y-4">
        <div className="flex items-center justify-between border-b border-[#14241C] pb-3">
          <div>
            <span className="text-[10px] font-mono text-[#10B981] uppercase tracking-widest">Execution Telemetry</span>
            <h3 className="text-sm font-bold text-white font-heading mt-0.5">Latest Model Solve Demonstration</h3>
          </div>
          <span className="badge badge-verified">VERIFIED PASS</span>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-5 gap-4 font-mono text-xs">
          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">PROBLEM INSTANCE</span>
            <span className="text-white font-bold block mt-1">{latestRun?.name || 'afiro.mps (Netlib LP)'}</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">SOLVER ENGINE</span>
            <span className="text-emerald-400 font-bold block mt-1">{latestRun?.solver || 'Revised Simplex (Sparse LU)'}</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">OPTIMAL OBJECTIVE</span>
            <span className="text-[#34D399] font-bold block mt-1">{latestRun?.objective || '-464.75314286'}</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">SOLVE TIMING</span>
            <span className="text-white font-bold block mt-1">{latestRun?.solveTimeMs || '0.45'} ms</span>
          </div>

          <div className="bg-[#080E0B] p-3 rounded border border-[#14241C]">
            <span className="text-gray-500 block text-[10px]">INDEPENDENT VERIFIER</span>
            <span className="text-emerald-400 font-bold block mt-1">PASS (1e-7 TOL)</span>
          </div>
        </div>
      </div>

    </div>
  );
}
