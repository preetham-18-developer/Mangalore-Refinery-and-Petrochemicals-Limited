import React, { useState } from 'react';
import { 
  FlaskConical, 
  Layers, 
  Cpu, 
  Database, 
  Activity, 
  ShieldCheck, 
  Zap, 
  CheckCircle2 
} from 'lucide-react';

export default function AlgorithmLab() {
  const [selectedAlg, setSelectedAlg] = useState('dual_simplex');

  const algorithms = [
    {
      id: 'presolve',
      name: 'Presolve Engine',
      tag: 'Matrix Reduction',
      purpose: 'Eliminate fixed variables, redundant constraints, singleton rows/cols, and perform bound tightening before main solve.',
      inputSpec: { m: 'm Constraints', n: 'n Variables', nnz: 'COO Matrix', density: 'Sparsity Pattern' },
      execution: { iterations: 'Up to 5 passes', basisUpdates: 'N/A', refactorisations: 'N/A', etaUpdates: 'N/A', runtime: '0.05 – 0.12 ms' },
      numerical: { primalResidual: '0.00e+00', dualResidual: '0.00e+00', feasibility: 'TIGHTENED', termination: 'FIXED_POINT_REACHED' }
    },
    {
      id: 'revised_simplex',
      name: 'Revised Simplex (Primal)',
      tag: 'Exact LP Solver',
      purpose: 'Solve general continuous LPs using primal-feasibility-preserving basis updates ($B^{-1}A$) and Bland anti-cycling pivot selection.',
      inputSpec: { m: 'm Rows', n: 'n Cols', nnz: 'Sparse COO/CSC', density: '< 15%' },
      execution: { iterations: '10 – 30 Pivots', basisUpdates: 'Sherman-Morrison / LU', refactorisations: 'Every 50 pivots', etaUpdates: 'Forrest-Tomlin', runtime: '0.45 – 1.20 ms' },
      numerical: { primalResidual: '< 1e-7', dualResidual: '< 1e-7', feasibility: 'FEASIBLE', termination: 'OPTIMAL' }
    },
    {
      id: 'dual_simplex',
      name: 'Dual Revised Simplex',
      tag: 'Warm-Start LP Solver',
      purpose: 'Solve LPs maintaining dual feasibility while driving primal infeasibilities to zero. Primary engine for MILP Branch-and-Bound relaxations.',
      inputSpec: { m: 'm Rows', n: 'n Cols', nnz: 'Sparse CSC', density: '< 10%' },
      execution: { iterations: '5 – 15 Dual Pivots', basisUpdates: 'Sparse LU Factor', refactorisations: 'On demand', etaUpdates: 'Product form', runtime: '0.25 – 0.85 ms' },
      numerical: { primalResidual: '< 1e-7', dualResidual: '0.00e+00', feasibility: 'DUAL_FEASIBLE', termination: 'OPTIMAL' }
    },
    {
      id: 'sparse_lu',
      name: 'Sparse LU Factorization',
      tag: 'Numerical Linear Algebra',
      purpose: 'Direct Sparse LU factorisation ($P A Q = L U$) with Markowitz threshold pivoting to maintain numerical stability and control fill-in.',
      inputSpec: { m: 'm x m Basis', n: 'm Cols', nnz: 'CSC Basis Matrix', density: '< 5%' },
      execution: { iterations: 'Markowitz Search', basisUpdates: 'Full Refactor', refactorisations: 'Active', etaUpdates: 'N/A', runtime: '0.12 – 0.35 ms' },
      numerical: { primalResidual: '< 1e-12 (LU Error)', dualResidual: 'N/A', feasibility: 'NON_SINGULAR', termination: 'FACTORIZED' }
    },
    {
      id: 'pdhg',
      name: 'PDHG / PDLP First-Order',
      tag: 'First-Order LP Kernel',
      purpose: 'Primal-Dual Hybrid Gradient algorithm solving massive sparse LPs via matrix-vector products without basis matrix inversions.',
      inputSpec: { m: 'm Rows (Large)', n: 'n Cols (Large)', nnz: '> 100,000', density: '< 1%' },
      execution: { iterations: '1,000+ Steps', basisUpdates: 'Matrix-Vector Product', refactorisations: 'None', etaUpdates: 'None', runtime: '12.5 – 45.0 ms' },
      numerical: { primalResidual: '< 1e-4', dualResidual: '< 1e-4', feasibility: 'APPROX_OPTIMAL', termination: 'CONVERGED' }
    },
    {
      id: 'branch_and_bound',
      name: 'Branch & Bound (MILP)',
      tag: 'Integer Optimizer',
      purpose: 'Solve Mixed-Integer Linear Programs (MILP) using dual simplex warm starts, pseudocost branching, and integer node pruning.',
      inputSpec: { m: 'm Constraints', n: 'n (Binary/Integer)', nnz: 'Sparse Matrix', density: '< 20%' },
      execution: { iterations: '14 – 48 Tree Nodes', basisUpdates: 'Child Basis Propagation', refactorisations: 'Per Node LP', etaUpdates: 'Active', runtime: '1.98 – 4.80 ms' },
      numerical: { primalResidual: '< 1e-7', dualResidual: '< 1e-7', feasibility: 'INTEGER_FEASIBLE', termination: 'GLOBAL_OPTIMAL' }
    },
    {
      id: 'warm_start',
      name: 'Warm Start Engine',
      tag: 'Basis Propagation',
      purpose: 'Propagate parent basis factorizations and bound modifications directly to child node LPs in B&B search trees.',
      inputSpec: { m: 'Parent Basis B', n: 'Bound Tightening', nnz: 'Sparse LU Factors', density: 'Preserved' },
      execution: { iterations: '0 Cold Pivots', basisUpdates: 'Basis Inherited', refactorisations: '0 Initial', etaUpdates: 'Reused', runtime: '< 0.05 ms' },
      numerical: { primalResidual: 'Inherited', dualResidual: '0.00e+00', feasibility: 'DUAL_FEASIBLE', termination: 'PROPAGATED' }
    },
    {
      id: 'adaptive_router',
      name: 'Adaptive Router',
      tag: 'Engine Dispatcher',
      purpose: 'Evaluate problem profiling features against hardware state to automatically select optimal continuous or integer solving engine.',
      inputSpec: { m: 'Dimension Vector', n: 'Density Metric', nnz: 'NNZ Count', density: 'Hardware State' },
      execution: { iterations: 'Feature Extraction', basisUpdates: 'N/A', refactorisations: 'N/A', etaUpdates: 'N/A', runtime: '< 0.01 ms' },
      numerical: { primalResidual: 'N/A', dualResidual: 'N/A', feasibility: 'PROFILED', termination: 'PATH_SELECTED' }
    }
  ];

  const current = algorithms.find(a => a.id === selectedAlg) || algorithms[0];

  return (
    <div className="space-y-6">
      
      {/* Header */}
      <div className="eng-card p-6 border-l-4 border-l-[#10B981]">
        <h2 className="text-xl font-bold text-white font-heading flex items-center gap-2">
          <FlaskConical className="w-5 h-5 text-[#10B981]" />
          Algorithm Lab
        </h2>
        <p className="text-xs text-gray-400 mt-1 font-sans">
          Inspect the numerical engines behind BHARATOPT: Presolve, Revised Simplex, Dual Simplex, Sparse LU, PDHG, Branch & Bound, Warm Start, and Adaptive Router.
        </p>
      </div>

      {/* Grid Layout: Algorithm Nav Left (4 cols), Spec Inspector Right (8 cols) */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* Left Nav */}
        <div className="lg:col-span-4 space-y-2">
          {algorithms.map((alg) => {
            const isSel = alg.id === selectedAlg;
            return (
              <div
                key={alg.id}
                onClick={() => setSelectedAlg(alg.id)}
                className={`p-3.5 rounded border cursor-pointer transition-all ${
                  isSel
                    ? 'bg-[#0E1D17] border-[#10B981]'
                    : 'bg-[#080E0B] border-[#14241C] hover:border-[#1C3327]'
                }`}
              >
                <div className="flex items-center justify-between text-xs font-mono">
                  <span className={`font-bold ${isSel ? 'text-white' : 'text-gray-300'}`}>{alg.name}</span>
                  <span className="text-[10px] font-semibold px-1.5 py-0.5 rounded bg-[#042F22] text-[#34D399]">
                    {alg.tag}
                  </span>
                </div>
              </div>
            );
          })}
        </div>

        {/* Right Workspace Inspector */}
        <div className="lg:col-span-8 space-y-6">
          <div className="eng-card p-6 space-y-6">
            
            {/* Title & Tag */}
            <div className="flex items-center justify-between border-b border-[#14241C] pb-4">
              <div>
                <span className="text-[10px] font-mono text-[#10B981] uppercase tracking-widest">Engine Workspace</span>
                <h3 className="text-lg font-bold text-white font-heading mt-0.5">{current.name}</h3>
              </div>
              <span className="badge badge-verified">{current.tag}</span>
            </div>

            {/* Purpose */}
            <div>
              <h4 className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider mb-1">Purpose</h4>
              <p className="text-xs text-gray-200 font-sans leading-relaxed bg-[#080E0B] p-3 rounded border border-[#14241C]">
                {current.purpose}
              </p>
            </div>

            {/* Input Structure */}
            <div>
              <h4 className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider mb-2">Input Structure</h4>
              <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
                {Object.entries(current.inputSpec).map(([key, val]) => (
                  <div key={key} className="bg-[#080E0B] p-2.5 rounded border border-[#14241C]">
                    <span className="text-gray-500 uppercase text-[9px] block">{key}</span>
                    <span className="text-white font-bold block mt-0.5">{val}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Execution Metrics */}
            <div>
              <h4 className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider mb-2">Execution Metrics</h4>
              <div className="grid grid-cols-2 md:grid-cols-5 gap-3 text-xs font-mono">
                {Object.entries(current.execution).map(([key, val]) => (
                  <div key={key} className="bg-[#080E0B] p-2.5 rounded border border-[#14241C]">
                    <span className="text-gray-500 uppercase text-[9px] block">{key}</span>
                    <span className="text-emerald-400 font-bold block mt-0.5">{val}</span>
                  </div>
                ))}
              </div>
            </div>

            {/* Numerical Status */}
            <div>
              <h4 className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider mb-2">Numerical Status</h4>
              <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
                {Object.entries(current.numerical).map(([key, val]) => (
                  <div key={key} className="bg-[#080E0B] p-2.5 rounded border border-[#14241C]">
                    <span className="text-gray-500 uppercase text-[9px] block">{key}</span>
                    <span className="text-[#34D399] font-bold block mt-0.5">{val}</span>
                  </div>
                ))}
              </div>
            </div>

          </div>
        </div>

      </div>

    </div>
  );
}
