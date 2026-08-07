import pandas as pd
import matplotlib.pyplot as plt
import os

files = {
    'Standard Nearest': 'standard_nearest.csv',
    'Standard Furthest': 'standard_furthest.csv',
    'Wrap Nearest': 'wrap_nearest.csv',
    'Wrap Furthest': 'wrap_furthest.csv'
}

def load_data(filename):
    if not os.path.exists(filename):
        print(f"Error: Can't find {filename}.")
        return None
    return pd.read_csv(filename, header=None)

fig, axes = plt.subplots(2, 2, figsize=(12, 10))
fig.suptitle('Distance Distributions (N=100,000)', fontsize=16)

df = load_data(files['Standard Nearest'])
if df is not None:
    axes[0, 0].hist(df[0], bins=50, color='skyblue', edgecolor='black')
    axes[0, 0].set_title('Standard: Nearest Neighbor')
    axes[0, 0].set_xlabel('Distance')
    axes[0, 0].set_ylabel('Count')

df = load_data(files['Standard Furthest'])
if df is not None:
    axes[0, 1].hist(df[0], bins=50, color='salmon', edgecolor='black')
    axes[0, 1].set_title('Standard: Furthest Neighbor')
    axes[0, 1].set_xlabel('Distance')

df = load_data(files['Wrap Nearest'])
if df is not None:
    axes[1, 0].hist(df[0], bins=50, color='lightgreen', edgecolor='black')
    axes[1, 0].set_title('Wraparound: Nearest Neighbor')
    axes[1, 0].set_xlabel('Distance')
    axes[1, 0].set_ylabel('Count')

df = load_data(files['Wrap Furthest'])
if df is not None:
    axes[1, 1].hist(df[0], bins=50, color='orange', edgecolor='black')
    axes[1, 1].set_title('Wraparound: Furthest Neighbor')
    axes[1, 1].set_xlabel('Distance')

plt.tight_layout(rect=[0, 0.03, 1, 0.95])
plt.savefig('distance_histograms.png') 
print("The graph saved as 'distance_histograms.png'.")
plt.show() 