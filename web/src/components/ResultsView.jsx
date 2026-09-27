import React, { useState } from 'react';
import { CheckCircle2, Clock, FileText, ArrowRight } from 'lucide-react';
import MathDisplay from './MathDisplay';

export default function ResultsView({ historyRecords, sampleModels }) {
  const defaultHistory = historyRecords && historyRecords.length > 0 ? historyRecords : [
    {
      id: 'afiro',
      name: 'afiro.mps (Netlib LP)',
      type: 'LP',
      status: 'OPTIMAL',
      objective: -464.75314286,
      solveTimeMs: 0.45,
      autoSelectedEngine: 'Dual Revised Simplex (CPU)',
      rows: 27,
      cols: 32,
      nnz: 88,
      density: '10.18%',
      verifierStatus: 'VERIFIED PASS',
      timestamp: 'Just now'
    },
    {
      id: 'p0033',
      name: 'p0033.mps (MIPLIB MILP)',
      type: 'MILP',
      status: 'OPTIMAL',
      objective: 3089.00,
      solveTimeMs: 2.15,
      autoSelectedEngine: 'Branch & Bound (Pseudocost)',
      rows: 16,
      cols: 33,
      nnz: 98,
      density: '18.56%',
      verifierStatus: 'VERIFIED PASS',
      timestamp: '5 mins ago'
    }
  ];

  const [selectedResult, setSelectedResult] = useState(defaultHistory[0]);

  return (
    <div className="max-w-[1000px] mx-auto space-y-6 py-4 font-sans">
      
      {/* Header */}
      <div className="space-y-1">
        <h2 className="text-xl font-bold text-white font-heading">Recent Solutions History</h2>
        <p className="text-xs text-gray-400">
          Review verified optimisation outcomes, extracted formulations, selected strategies, and timing telemetry.
        </p>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        
        {/* Left List of Solved Items */}
        <div className="lg:col-span-4 space-y-2 font-mono text-xs">
          <span className="text-gray-500 uppercase text-[10px] block">Previous Solves:</span>
          {defaultHistory.map((item, idx) => {
            const isSel = selectedResult?.name === item.name;
            return (
              <div
                key={idx}
                onClick={() => setSelectedResult(item)}
                className={`p-3.5 rounded border cursor-pointer transition-all ${
                  isSel 
                    ? 'bg-[#0E1D17] border-[#10B981]' 
                    : 'bg-[#080E0B] border-[#14241C] hover:border-[#1C3327]'
                }`}
              >
                <div className="flex items-center justify-between">
                  <span className="font-bold text-white">{item.name}</span>
                  <span className="text-emerald-400 text-[10px]">{item.type}</span>
                </div>
                <div className="flex items-center justify-between text-[11px] text-gray-400 mt-2">
                  <span className="text-[#34D399] font-bold">{item.objective}</span>
                  <span>{item.solveTimeMs} ms</span>
                </div>
              </div>
            );
          })}
        </div>

        {/* Right Result Inspector */}
        <div className="lg:col-span-8 space-y-6">
          {selectedResult && (
            <div className="eng-card p-6 space-y-6 bg-[#0A1410]">
              
              <div className="flex items-center justify-between border-b border-[#14241C] pb-3">
                <div>
                  <span className="text-[10px] font-mono text-gray-500 uppercase block">Solution Inspector</span>
                  <h3 className="text-lg font-bold text-white font-heading">{selectedResult.name}</h3>
                </div>
                <span className="badge badge-verified">{selectedResult.verifierStatus}</span>
              </div>

              <div className="grid grid-cols-2 gap-4 font-mono text-xs">
                <div className="bg-[#060A08] p-4 rounded border border-[#14241C]">
                  <span className="text-gray-500 block uppercase text-[10px]">Optimal Objective</span>
                  <span className="text-2xl font-bold text-[#34D399] block mt-1">{selectedResult.objective}</span>
                </div>

                <div className="bg-[#060A08] p-4 rounded border border-[#14241C]">
                  <span className="text-gray-500 block uppercase text-[10px]">Solve Runtime</span>
                  <span className="text-2xl font-bold text-white block mt-1">{selectedResult.solveTimeMs} ms</span>
                </div>
              </div>

              <div className="bg-[#080E0B] p-4 rounded border border-[#14241C] font-mono text-xs space-y-1">
                <span className="text-gray-500 text-[10px] uppercase block">Selected Method</span>
                <div className="text-white font-bold">{selectedResult.autoSelectedEngine}</div>
                <div className="text-gray-400 text-[11px]">{selectedResult.rows} constraints • {selectedResult.cols} variables • {selectedResult.nnz} NNZ</div>
              </div>

              <MathDisplay model={sampleModels ? sampleModels[selectedResult.id] : null} />

            </div>
          )}
        </div>

      </div>

    </div>
  );
}
