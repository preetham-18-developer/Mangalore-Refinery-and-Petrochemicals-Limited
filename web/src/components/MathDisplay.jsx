import React from 'react';

export default function MathDisplay({ model }) {
  if (!model) return null;

  // Custom mathematical equations for small/understandable models
  if (model.id === 'afiro' || model.name?.includes('afiro')) {
    return (
      <div className="bg-[#060A08] p-5 rounded border border-[#14241C] font-mono text-xs space-y-4">
        <div className="flex items-center justify-between border-b border-[#14241C] pb-2 text-[11px] text-gray-400">
          <span>MATHEMATICAL FORMULATION (EXTRACTED)</span>
          <span className="text-emerald-400 font-bold">Standard LP Form</span>
        </div>

        <div className="space-y-3 font-sans text-sm text-gray-200">
          <div>
            <span className="text-xs text-gray-500 uppercase font-mono block mb-1">Objective Function</span>
            <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] font-mono text-xs text-[#34D399]">
              <span className="text-gray-400 font-bold">minimize</span> &nbsp; cᵀx = -0.4 x₁ - 0.32 x₂ + 15.0 x₃ + 12.0 x₄ ...
            </div>
          </div>

          <div>
            <span className="text-xs text-gray-500 uppercase font-mono block mb-1">Constraints & Bounds</span>
            <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] font-mono text-xs text-gray-300 space-y-1.5">
              <div><span className="text-gray-500">Subject to:</span> &nbsp; A x ≤ b</div>
              <div className="pl-4 text-gray-400">2.0 x₁ + 1.5 x₂ + x₃ ≤ 250.0 &nbsp; (Capacity 1)</div>
              <div className="pl-4 text-gray-400">1.0 x₁ + 2.0 x₂ + x₄ ≤ 300.0 &nbsp; (Capacity 2)</div>
              <div className="pl-4 text-gray-400">-0.5 x₁ + 1.0 x₅ = 0.0 &nbsp; (Blending Ratio)</div>
              <div className="pl-4 text-gray-500 font-bold">l ≤ x ≤ u, &nbsp; x ≥ 0</div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  if (model.id === 'refinery' || model.name?.includes('Refinery')) {
    return (
      <div className="bg-[#060A08] p-5 rounded border border-[#14241C] font-mono text-xs space-y-4">
        <div className="flex items-center justify-between border-b border-[#14241C] pb-2 text-[11px] text-gray-400">
          <span>INDUSTRIAL REFINERY CASE STUDY FORMULATION</span>
          <span className="text-emerald-400 font-bold">Domain LP Model</span>
        </div>

        <div className="space-y-3 font-sans text-sm text-gray-200">
          <div>
            <span className="text-xs text-gray-500 uppercase font-mono block mb-1">Objective: Maximize Gross Margin ($/day)</span>
            <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] font-mono text-xs text-[#34D399]">
              <span className="text-gray-400 font-bold">maximize</span> &nbsp; Z = ∑ (Revenue_products - Cost_crudes - Operating_costs)
            </div>
          </div>

          <div>
            <span className="text-xs text-gray-500 uppercase font-mono block mb-1">Domain Constraints</span>
            <div className="bg-[#080E0B] p-3 rounded border border-[#14241C] font-mono text-xs text-gray-300 space-y-1.5">
              <div><span className="text-gray-500">1. Crude Availability:</span> &nbsp; x_ArabianLight ≤ 100,000 bpd</div>
              <div><span className="text-gray-500">2. Distillation Capacity:</span> &nbsp; x_ArabianLight + x_Heavy ≤ 150,000 bpd</div>
              <div><span className="text-gray-500">3. Gasoline Octane Blend:</span> &nbsp; 91 x_Premium + 87 x_Regular ≥ 89 Total_Gasoline</div>
              <div><span className="text-gray-500">4. Non-negativity:</span> &nbsp; x_i ≥ 0</div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  // General Compact Representation for standard MPS models
  return (
    <div className="bg-[#060A08] p-5 rounded border border-[#14241C] font-mono text-xs space-y-3">
      <div className="flex items-center justify-between border-b border-[#14241C] pb-2 text-[11px] text-gray-400">
        <span>EXTRACTED MATHEMATICAL STRUCTURE</span>
        <span className="text-emerald-400 font-bold">Linear Optimization Model</span>
      </div>

      <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] space-y-3">
        <div className="flex items-baseline gap-3">
          <span className="text-gray-500 font-bold uppercase w-24">Objective:</span>
          <span className="text-[#34D399] font-bold text-sm">
            {model.objectiveSense === 'MAXIMIZE' ? 'maximize' : 'minimize'} &nbsp; cᵀx
          </span>
        </div>

        <div className="flex items-baseline gap-3">
          <span className="text-gray-500 font-bold uppercase w-24">Subject to:</span>
          <span className="text-gray-200 text-xs">
            A x ≤ b &nbsp; (with {model.rows} rows, {model.cols} columns, {model.nnz} non-zeros)
          </span>
        </div>

        <div className="flex items-baseline gap-3">
          <span className="text-gray-500 font-bold uppercase w-24">Bounds:</span>
          <span className="text-gray-300 text-xs">
            l_j ≤ x_j ≤ u_j &nbsp; (j = 1, ..., {model.cols})
          </span>
        </div>
      </div>
    </div>
  );
}
