import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

# 1. Check arguments
if len(sys.argv) < 2:
    print("❌ Error: No CSV file provided.")
    print("Usage: python3 csv_plot.py <csv_file>")
    sys.exit(1)

csv_file = sys.argv[1]

try:
    # 2. Read CSV
    df = pd.read_csv(csv_file)
    print(f"✅ Successfully loaded: {csv_file}")
    
    # Strip whitespace from column names just in case
    df.columns = df.columns.str.strip()
    
    print("Columns found:", df.columns.tolist())

    # 3. Setup the Plot
    fig, ax1 = plt.subplots(figsize=(10, 6))

    # --- Plot Throughput (TPS) on Left Y-Axis ---
    color = 'tab:blue'
    ax1.set_xlabel('VUs (Virtual Users)')
    ax1.set_ylabel('Throughput (TPS)', color=color)
    
    # Plot the line
    ax1.plot(df["vus"], df["tps"], marker='o', color=color, label='TPS')
    ax1.tick_params(axis='y', labelcolor=color)
    ax1.grid(True, linestyle='--', alpha=0.6)

    # --- Plot Latency (Avg) on Right Y-Axis ---
    ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis
    color = 'tab:red'
    ax2.set_ylabel('Avg Latency (ms)', color=color)
    
    # Plot the line
    ax2.plot(df["vus"], df["avg_ms"], marker='x', linestyle='--', color=color, label='Avg Latency')
    ax2.tick_params(axis='y', labelcolor=color)

    # --- ### NEW CODE: Add CPU Annotations ### ---
    # We check if the 'cpu' column exists (based on your snippet, it seems to be named 'cpu' at the end)
    # If your CSV header says 'cpu_util_pct', change 'cpu' below to that. 
    # Your snippet showed both, but the last column was 'cpu'.
    cpu_col = 'cpu' 
    
    if cpu_col in df.columns:
        for i, row in df.iterrows():
            # Get coordinates
            x = row['vus']
            y = row['tps']
            cpu_val = row[cpu_col]
            
            # Annotate on the TPS line (ax1)
            ax1.annotate(
                f"CPU:{cpu_val}%", 
                (x, y), 
                textcoords="offset points", 
                xytext=(0, 10), # 10 points vertical offset
                ha='center', 
                fontsize=8,
                color='black',
                fontweight='bold'
            )
    else:
        print(f"⚠️ Warning: Column '{cpu_col}' not found. Skipping annotations.")

    # Title and Layout
    plt.title(f'Load Test Results: {os.path.basename(csv_file)}')
    fig.tight_layout() 

    # 4. Save the plot to /results folder
    results_dir = "results"
    os.makedirs(results_dir, exist_ok=True)
    
    base_name = os.path.basename(csv_file)
    image_name = base_name.replace('.csv', '.png')
    output_path = os.path.join(results_dir, image_name)
    
    plt.savefig(output_path)
    print(f"📈 Plot saved to: {output_path}")

except KeyError as e:
    print(f"\n❌ Column name error: {e}")
    print(f"   Available columns are: {list(df.columns)}")
    print("   Please update the script to match these column names.")
except Exception as e:
    print(f"❌ An error occurred: {e}")