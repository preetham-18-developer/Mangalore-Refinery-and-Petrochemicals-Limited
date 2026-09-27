import React from 'react';
import { 
  Activity, 
  Cpu, 
  Database, 
  BarChart3, 
  ShieldCheck, 
  Zap, 
  Layers,
  FlaskConical
} from 'lucide-react';

export default function Navbar({ activeTab, setActiveTab }) {
  const tabs = [
    { id: 'solver', label: 'Solver & Model Studio', icon: Zap },
    { id: 'router', label: 'Adaptive Router', icon: Cpu },
    { id: 'scalability', label: 'Scalability Engine (10⁶)', icon: Database },
    { id: 'ablation', label: 'Ablation Suite (A1–A7)', icon: FlaskConical },
    { id: 'stress', label: 'Numerical Stress Suite', icon: Activity },
    { id: 'telemetry', label: 'Telemetry & Gate Vault', icon: ShieldCheck }
  ];

  return (
    <header className="border-b border-[var(--border-color)] bg-[#0B0F19]/90 backdrop-blur-md sticky top-0 z-50">
      <div className="max-w-7xl mx-auto px-6 py-4 flex flex-col md:flex-row md:items-center justify-between gap-4">
        
        {/* Brand & Badges */}
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-[#00F2FE] to-[#8B5CF6] flex items-center justify-center font-bold text-slate-900 text-xl shadow-lg shadow-cyan-500/20">
            β
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h1 className="text-xl font-extrabold text-white tracking-tight">BHARATOPT</h1>
              <span className="px-2 py-0.5 text-[10px] font-bold bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 rounded-full uppercase tracking-wider">
                SIH 2026 PS 26119
              </span>
            </div>
            <p className="text-xs text-slate-400 font-medium">
              Adaptive Indigenous CPU–GPU Optimization Solver Engine | Target: <span className="text-slate-200 font-semibold">MRPL</span>
            </p>
          </div>
        </div>

        {/* Live Status Diagnostics */}
        <div className="flex items-center gap-3 text-xs font-mono">
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-emerald-500/10 border border-emerald-500/30 text-emerald-400">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse"></span>
            CPU Simplex: ACTIVE
          </div>
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-purple-500/10 border border-purple-500/30 text-purple-300">
            <span className="w-2 h-2 rounded-full bg-purple-400"></span>
            Verifier: 100% PASS
          </div>
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-amber-500/10 border border-amber-500/30 text-amber-300">
            <span className="w-2 h-2 rounded-full bg-amber-400"></span>
            CUDA: CPU_FALLBACK
          </div>
        </div>

      </div>

      {/* Tab Navigation */}
      <div className="max-w-7xl mx-auto px-6 flex items-center gap-2 overflow-x-auto no-scrollbar border-t border-[var(--border-color)]/50 pt-2">
        {tabs.map((t) => {
          const Icon = t.icon;
          const isActive = activeTab === t.id;
          return (
            <button
              key={t.id}
              onClick={() => setActiveTab(t.id)}
              className={`flex items-center gap-2 px-4 py-2.5 text-xs font-semibold rounded-t-lg transition-all border-b-2 whitespace-nowrap ${
                isActive
                  ? 'border-[#00F2FE] text-[#00F2FE] bg-cyan-500/10'
                  : 'border-transparent text-slate-400 hover:text-slate-200 hover:bg-slate-800/40'
              }`}
            >
              <Icon className="w-4 h-4" />
              {t.label}
            </button>
          );
        })}
      </div>
    </header>
  );
}
