import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import sys
import os


def plot_benchmarks(csv_file):

    df = pd.read_csv(csv_file)

    ind = ["Structure", "ISA", "Type", "B", "N"]

    baseline_df = df[df["ISA"] == "STL"][["N", "cpu_mean", "Type", "var"]]
    baseline_df = baseline_df.rename(
        columns={"cpu_mean": "baseline_time", "var": "baseline_var"}
    )
    # Merge the baseline times back into the main dataframe
    df = df.merge(baseline_df, on=["N", "Type"], how="left")
    df["speedup"] = df["baseline_time"] / df["cpu_mean"]
    df["mad"] = df["speedup"] * pd.Series(
        [
            np.sqrt(
                (mad_x) ** 2 + (mad_y) ** 2
            )  # relative formula for error propagation
            for x, mad_x, y, mad_y in zip(
                df["cpu_mean"], df["var"], df["baseline_time"], df["baseline_var"]
            )
        ]
    )
    print(df)
    # 3. Setup Plotting Aesthetics
    sns.set_theme(style="whitegrid")
    print(df.groupby("mask").count())
    max_idx = df.groupby(by=["Type"])["speedup"].transform("max") == df["speedup"]
    max_df = (
        df[max_idx][["Type", "ISA", "B", "speedup", "N"]]
        .reset_index()
        .set_index("Type")[["ISA", "B", "speedup", "N"]]
        .sort_values("Type")
    )
    max_df["Device"] = os.path.basename(sys.argv[1])
    max_df.to_csv(f"{sys.argv[2]}/max.csv")
    print(max_df.to_markdown())
    for dt in df["Type"].unique():
        # 4. Create the Plot
        # We'll plot Size on X and Time on Y, using 'ISA' as the color hue
        subdf = df[df["Type"] == dt]
        fig, ax = plt.subplots(figsize=(12, 6))
        ax.errorbar(
            x=subdf["N"],
            y=subdf["speedup"],
            yerr=subdf["mad"],
            fmt="none",
            c="black",
            capsize=3,
        )
        plot = sns.linemarkerplot = sns.lineplot(
            data=df[df["Type"] == dt],
            x="N",
            y="speedup",
            hue="ISA",
            style="B",
            markers=True,
            markersize=12,
            dashes=False,
            ax=ax,
        )
        # Formatting
        plt.title(f"Benchmark Performance by ISA and B (datatype:{dt})", fontsize=15)
        plt.xlabel("Input Size (N)", fontsize=12)
        plt.ylabel(f"Speed up", fontsize=12)
        if "8" not in dt:
            ax.set_xscale("log", base=2)  # Often useful for benchmark sizes
        # plt.yscale("log")
        plt.legend(title="Configurations", bbox_to_anchor=(1, 0), loc="lower left")
        plt.tight_layout()

        # 5. Save/Show
        plt.savefig(f"{sys.argv[2]}/benchmark_results_{dt}.png", dpi=300)


if __name__ == "__main__":
    plot_benchmarks(sys.argv[1])
