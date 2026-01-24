#!/usr/bin/env python3
"""sqrt optimization benchmark: baseline vs current"""

import subprocess
import sys
import re
import time
from pathlib import Path

PROJECT = Path(__file__).parent
BUILD = PROJECT / "build_bench"
BUILD.mkdir(exist_ok=True)

def run(cmd, desc):
    print(f"→ {desc}... ", end='', flush=True)
    start = time.time()
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=BUILD)
    elapsed = time.time() - start
    if result.returncode != 0:
        print(f"✗ ({elapsed:.1f}s)")
        print(result.stderr)
        return None
    print(f"✓ ({elapsed:.1f}s)")
    return result.stdout

def compile_bench(name, flags):
    cmd = f'g++ -std=c++14 -O3 -march=native {flags} -I{PROJECT}/include {PROJECT}/test/benchmark_sqrt.cpp -o {BUILD}/bench_{name}'
    return run(cmd, f"Compile {name}")

def run_bench(name):
    return run(f'{BUILD}/bench_{name}', f"Run {name}")

def parse(text):
    results = {}
    pattern = r'(\w+(?:_\w+)*) Results:.*?Throughput:\s+([\d.]+) ops/sec.*?Avg time/op:\s+([\d.]+)'
    for m in re.finditer(pattern, text, re.DOTALL):
        results[m.group(1)] = {'throughput': float(m.group(2)), 'time_us': float(m.group(3))}
    return results

print("\n" + "="*80)
print("  sqrt Optimization: Baseline vs Current")
print("="*80 + "\n")

# Compile baseline
print("\n→ Compiling baseline... ", end='', flush=True)
baseline_cpp = BUILD / "benchmark_baseline.cpp"
with open(PROJECT / "test/benchmark_sqrt.cpp") as f:
    code = f.read()
code = code.replace('#include <boost/decimal.hpp>', 
                   '#include <boost/decimal.hpp>\n#include <boost/decimal/detail/cmath/sqrt_baseline.hpp>')
code = code.replace('sqrt(x)', 'sqrt_baseline(x)')
with open(baseline_cpp, 'w') as f:
    f.write(code)

result = subprocess.run(
    f'g++ -std=c++14 -O3 -march=native -I{PROJECT}/include {baseline_cpp} -o {BUILD}/bench_baseline',
    shell=True, capture_output=True, text=True
)
if result.returncode == 0:
    print("✓")
else:
    print("✗")
    print(result.stderr)
    sys.exit(1)

# Compile current
compile_bench("current", "")

# Run benchmarks
print()
out_baseline = run_bench("baseline")
out_current = run_bench("current")

if not out_baseline or not out_current:
    sys.exit(1)

data_baseline = parse(out_baseline)
data_current = parse(out_current)

# Results
print("\n" + "="*80)
print("  Results: Baseline vs Current")
print("="*80)
print(f"\n{'Type':<20} {'Baseline':<15} {'Current':<15} {'Speedup':<10}")
print("-"*80)

types = ['decimal32_t', 'decimal64_t', 'decimal128_t', 
         'decimal_fast32_t', 'decimal_fast64_t', 'decimal_fast128_t']

for t in types:
    if t in data_baseline and t in data_current:
        b = data_baseline[t]['throughput']
        c = data_current[t]['throughput']
        speedup = c / b
        if speedup > 1.01:  # >1% improvement
            marker = "✓"
            pct = f"+{(speedup-1)*100:.1f}%"
        elif speedup < 0.99:  # >1% regression
            marker = "✗"
            pct = f"{(speedup-1)*100:.1f}%"
        else:
            marker = "="
            pct = "~same"
        print(f"{t:<20} {b/1e6:>6.2f}M ops/s  {c/1e6:>6.2f}M ops/s  {marker} {speedup:>4.2f}x {pct}")

print("\n" + "="*80)
if any(t in data_current and data_current[t]['throughput'] > data_baseline[t]['throughput'] * 1.01
       for t in types if t in data_baseline):
    print("✓ Performance improved! 🎉")
elif any(t in data_current and data_current[t]['throughput'] < data_baseline[t]['throughput'] * 0.99
         for t in types if t in data_baseline):
    print("✗ Performance regressed! Revert changes.")
else:
    print("= No significant change (baseline == current)")
print("="*80 + "\n")
