import matplotlib.pyplot as plt
import numpy as np

data = {
    'Standard Geometry': {
        'Naive': {
            'Static': {'time': 16.56, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909},
            'Dynamic': {'time': 19.24, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909},
            'Guided': {'time': 21.1837, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909}
        },
        'Optimized': {
            'Static': {'time': 23.24, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909},
            'Dynamic': {'time': 18.13, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909},
            'Guided': {'time': 18.70, 'avg_nearest': 0.00158, 'avg_furthest': 1.06909}
        }
    },
    'Wraparound Geometry': {
        'Naive': {
            'Static': {'time': 42.99, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571},
            'Dynamic': {'time': 43.08, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571},
            'Guided': {'time': 43.04, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571}
        },
        'Optimized': {
            'Static': {'time': 42.01, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571},
            'Dynamic': {'time': 29.39, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571},
            'Guided': {'time': 30.52, 'avg_nearest': 0.00158, 'avg_furthest': 0.70571}
        }
    }
}

num_points = 100000
num_threads = 8  


def plot_execution_times():
    """Compare execution times across all methods"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    schedules = ['Static', 'Dynamic', 'Guided']
    x = np.arange(len(schedules))
    width = 0.35
    
    # Standard Geometry
    naive_times_std = [data['Standard Geometry']['Naive'][s]['time'] for s in schedules]
    opt_times_std = [data['Standard Geometry']['Optimized'][s]['time'] for s in schedules]
    
    ax1.bar(x - width/2, naive_times_std, width, label='Naive', alpha=0.8, color='#3498db')
    ax1.bar(x + width/2, opt_times_std, width, label='Optimized', alpha=0.8, color='#e74c3c')
    
    ax1.set_xlabel('Schedule Type', fontsize=11)
    ax1.set_ylabel('Execution Time (seconds)', fontsize=11)
    ax1.set_title('Standard Geometry Performance', fontsize=12, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(schedules)
    ax1.legend()
    ax1.grid(axis='y', alpha=0.3)
    
    # Wraparound Geometry
    naive_times_wrap = [data['Wraparound Geometry']['Naive'][s]['time'] for s in schedules]
    opt_times_wrap = [data['Wraparound Geometry']['Optimized'][s]['time'] for s in schedules]
    
    ax2.bar(x - width/2, naive_times_wrap, width, label='Naive', alpha=0.8, color='#3498db')
    ax2.bar(x + width/2, opt_times_wrap, width, label='Optimized', alpha=0.8, color='#e74c3c')
    
    ax2.set_xlabel('Schedule Type', fontsize=11)
    ax2.set_ylabel('Execution Time (seconds)', fontsize=11)
    ax2.set_title('Wraparound Geometry Performance', fontsize=12, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(schedules)
    ax2.legend()
    ax2.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('execution_time_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: execution_time_comparison.png")
    plt.show()

def plot_schedule_comparison():
    """Compare different schedules for each algorithm"""
    fig, ax = plt.subplots(figsize=(10, 6))
    
    schedules = ['Static', 'Dynamic', 'Guided']
    x = np.arange(len(schedules))
    width = 0.2
    
    naive_std = [data['Standard Geometry']['Naive'][s]['time'] for s in schedules]
    naive_wrap = [data['Wraparound Geometry']['Naive'][s]['time'] for s in schedules]
    opt_std = [data['Standard Geometry']['Optimized'][s]['time'] for s in schedules]
    opt_wrap = [data['Wraparound Geometry']['Optimized'][s]['time'] for s in schedules]
    
    ax.bar(x - 1.5*width, naive_std, width, label='Naive (Standard)', alpha=0.8, color='#3498db')
    ax.bar(x - 0.5*width, naive_wrap, width, label='Naive (Wraparound)', alpha=0.8, color='#2ecc71')
    ax.bar(x + 0.5*width, opt_std, width, label='Optimized (Standard)', alpha=0.8, color='#e74c3c')
    ax.bar(x + 1.5*width, opt_wrap, width, label='Optimized (Wraparound)', alpha=0.8, color='#f39c12')
    
    ax.set_xlabel('Schedule Type', fontsize=12)
    ax.set_ylabel('Execution Time (seconds)', fontsize=12)
    ax.set_title('Complete Schedule Comparison Across All Implementations', fontsize=13, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(schedules)
    ax.legend(loc='upper right')
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('schedule_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: schedule_comparison.png")
    plt.show()

def plot_geometry_comparison():
    """Compare Standard vs Wraparound geometry impact on distances"""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    std_nearest = data['Standard Geometry']['Naive']['Static']['avg_nearest']
    wrap_nearest = data['Wraparound Geometry']['Naive']['Static']['avg_nearest']
    
    categories = ['Standard', 'Wraparound']
    nearest_vals = [std_nearest, wrap_nearest]
    furthest_vals = [
        data['Standard Geometry']['Naive']['Static']['avg_furthest'],
        data['Wraparound Geometry']['Naive']['Static']['avg_furthest']
    ]
    
    x = np.arange(len(categories))
    width = 0.35
    
    ax1.bar(x, nearest_vals, width, color=['#3498db', '#2ecc71'], alpha=0.8)
    ax1.set_ylabel('Average Distance', fontsize=11)
    ax1.set_title('Average Nearest Neighbor Distance', fontsize=12, fontweight='bold')
    ax1.set_xticks(x)
    ax1.set_xticklabels(categories)
    ax1.grid(axis='y', alpha=0.3)
    
    diff_nearest = ((std_nearest - wrap_nearest) / std_nearest) * 100
    ax1.text(0.5, max(nearest_vals) * 0.95, f'{diff_nearest:.1f}% reduction', 
             ha='center', fontsize=10, fontweight='bold', 
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    ax2.bar(x, furthest_vals, width, color=['#e74c3c', '#f39c12'], alpha=0.8)
    ax2.set_ylabel('Average Distance', fontsize=11)
    ax2.set_title('Average Furthest Neighbor Distance', fontsize=12, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(categories)
    ax2.grid(axis='y', alpha=0.3)
    
    diff_furthest = ((furthest_vals[0] - furthest_vals[1]) / furthest_vals[0]) * 100
    ax2.text(0.5, max(furthest_vals) * 0.95, f'{diff_furthest:.1f}% reduction', 
             ha='center', fontsize=10, fontweight='bold',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    plt.savefig('geometry_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: geometry_comparison.png")
    plt.show()


if __name__ == "__main__":
    print("\n" + "="*60)
    print("GENERATING PERFORMANCE COMPARISON VISUALIZATIONS")
    print("="*60 + "\n")
    
    print("Creating visualizations...")
    print("-" * 60)
    
    plot_execution_times()
    plot_schedule_comparison()
    plot_geometry_comparison()
    
    print("\n" + "="*60)
    print("ALL VISUALIZATIONS COMPLETED")
    print("="*60)
    print("\nGenerated files:")
    print("  1. execution_time_comparison.png")
    print("  2. schedule_comparison.png")
    print("  3. geometry_comparison.png")