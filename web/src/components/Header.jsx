import React from 'react';
import { ArrowRight } from 'lucide-react';

export default function Header({ activeTab, setActiveTab, onSolveProblem }) {
  const tabs = [
    { id: 'solve', label: 'Solve' },
    { id: 'results', label: 'Results' },
    { id: 'benchmarks', label: 'Benchmarks' }
  ];

  return (
    <header className="border-b border-[#14241C] bg-[#070E0B]/95 sticky top-0 z-50 backdrop-blur-md">
      <div className="max-w-[1100px] mx-auto px-6 h-16 flex items-center justify-between gap-6 font-sans">
        
        {/* Left: Brand & Subtitle */}
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded bg-[#047857] border border-[#10B981] flex items-center justify-center font-bold text-white text-base font-heading">
            β
          </div>
          <div>
            <h1 className="text-base font-bold text-white tracking-tight font-heading leading-none">BHARATOPT</h1>
            <p className="text-[11px] text-gray-400 font-mono mt-0.5">Optimization Engine</p>
          </div>
        </div>

        {/* Center: 3 Navigation Tabs */}
        <nav className="flex items-center gap-1">
          {tabs.map((t) => {
            const isActive = activeTab === t.id;
            return (
              <button
                key={t.id}
                onClick={() => setActiveTab(t.id)}
                className={`nav-tab px-4 py-2 text-xs font-medium rounded transition-colors ${
                  isActive
                    ? 'active text-white bg-[#0A1812]'
                    : 'text-gray-400 hover:text-gray-200 hover:bg-[#080E0B]'
                }`}
              >
                {t.label}
              </button>
            );
          })}
        </nav>

        {/* Right: Primary CTA Action */}
        <div>
          <button 
            onClick={onSolveProblem}
            className="btn-emerald text-xs px-4 py-2"
          >
            Solve a Problem
            <ArrowRight className="w-3.5 h-3.5" />
          </button>
        </div>

      </div>
    </header>
  );
}
