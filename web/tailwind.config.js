/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        bg: {
          primary: '#040706',
          secondary: '#080F0C',
          card: '#0D1612',
          cardHover: '#121F19',
          cardActive: '#16261F',
          inset: '#060A08',
        },
        border: {
          subtle: '#14241C',
          medium: '#1C3327',
          emerald: '#059669',
        },
        accent: {
          emerald: '#10B981',
          emeraldBright: '#34D399',
          emeraldDark: '#047857',
          emeraldDeep: '#064E3B',
        }
      },
      fontFamily: {
        sans: ['Inter', '-apple-system', 'BlinkMacSystemFont', 'Segoe UI', 'Roboto', 'sans-serif'],
        heading: ['Outfit', 'Inter', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
      }
    },
  },
  plugins: [],
}
