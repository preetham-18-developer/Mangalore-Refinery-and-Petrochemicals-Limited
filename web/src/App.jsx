import React, { useState } from 'react';
import Header from './components/Header';
import SolveView from './components/SolveView';
import ResultsView from './components/ResultsView';
import BenchmarksView from './components/BenchmarksView';

export default function App() {
  const [activeTab, setActiveTab] = useState('solve');
  const [historyRecords, setHistoryRecords] = useState([]);

  // Benchmark Models Library
  const sampleModels = {
    afiro: {
      id: 'afiro',
      name: 'AFIRO',
      type: 'Linear Programming',
      cols: 32,
      rows: 27,
      nnz: 88,
      density: '10.18%',
      objectiveSense: 'MINIMIZE',
      expectedObj: -464.75314286,
      solver: 'Dual Revised Simplex · CPU',
      presolveStats: { rowsElim: 12, colsElim: 8, timeMs: 0.05 },
      solveTimeMs: 0.45,
      iterations: 10,
      verifierStatus: 'VERIFIED'
    },
    p0033: {
      id: 'p0033',
      name: 'P0033',
      type: 'Mixed-Integer Linear Programming',
      cols: 33,
      rows: 16,
      nnz: 98,
      density: '18.56%',
      objectiveSense: 'MINIMIZE',
      expectedObj: 3089.0,
      solver: 'Branch & Bound · CPU',
      presolveStats: { rowsElim: 4, colsElim: 5, timeMs: 0.10 },
      solveTimeMs: 2.15,
      iterations: 14,
      verifierStatus: 'VERIFIED'
    },
    blend2: {
      id: 'blend2',
      name: 'BLEND2',
      type: 'Mixed-Integer Linear Programming',
      cols: 357,
      rows: 36,
      nnz: 1124,
      density: '8.74%',
      objectiveSense: 'MAXIMIZE',
      expectedObj: 3089.0,
      solver: 'Branch & Bound · CPU',
      presolveStats: { rowsElim: 8, colsElim: 15, timeMs: 0.12 },
      solveTimeMs: 1.98,
      iterations: 14,
      verifierStatus: 'VERIFIED'
    },
    ill_cond_hilbert: {
      id: 'ill_cond_hilbert',
      name: 'HILBERT-5',
      type: 'Numerical Stress Test',
      cols: 5,
      rows: 5,
      nnz: 25,
      density: '100.0%',
      objectiveSense: 'MINIMIZE',
      expectedObj: 5.0,
      solver: 'Revised Simplex (Double Precision)',
      presolveStats: { rowsElim: 0, colsElim: 0, timeMs: 0.0 },
      solveTimeMs: 0.28,
      iterations: 5,
      verifierStatus: 'VERIFIED'
    }
  };

  const handleSolveRecorded = (record) => {
    setHistoryRecords(prev => [record, ...prev]);
  };

  return (
    <div className="min-h-screen bg-[#040706] text-gray-100 font-sans pb-16">
      
      {/* Primary Minimal Header */}
      <Header 
        activeTab={activeTab} 
        setActiveTab={setActiveTab} 
        onSolveProblem={() => setActiveTab('solve')}
      />

      {/* Main Workspace */}
      <main className="max-w-[1100px] mx-auto px-6 pt-4">
        
        {activeTab === 'solve' && (
          <SolveView 
            sampleModels={sampleModels}
            onSolveComplete={handleSolveRecorded}
            onNavigateToBenchmarks={() => setActiveTab('benchmarks')}
          />
        )}

        {activeTab === 'results' && (
          <ResultsView 
            historyRecords={historyRecords}
            sampleModels={sampleModels}
          />
        )}

        {activeTab === 'benchmarks' && (
          <BenchmarksView />
        )}

      </main>

    </div>
  );
}
