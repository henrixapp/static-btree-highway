import pandas as pd
import json
import seaborn as sns
import matplotlib.pyplot as plt
import sys


def plot_benchmarks(json_file):
    df = None
    if "json" in json_file:
        # 1. Load the Google Benchmark JSON
        with open(json_file, "r") as f:
            data = json.load(f)

        df = pd.DataFrame(data["benchmarks"])

        df = df[
            ~(df["name"].str.contains("bench::BestImplementation<int>/100"))
            & df["name"].str.contains("00_")
        ]
    else:
        df = pd.read_csv(json_file)

    # 2. Parse the complex name column
    # Pattern: "StaticBTree,N_AVX10_2,uint8_t,128/100"
    # We split by ',' and then handle the last part which contains the size/iterations
    def parse_name(name: str):
        parts = name.split(",")
        data = parts[-1].split("/")[1].split("_")[1]
        print(name, data, parts)
        if len(parts) == 4:
            structure = parts[0]
            isa = parts[1]
            dtype = parts[2]
            # Split the last part (e.g., "128/100") to get the size
            size = parts[3].split("/")[0]
            return pd.Series(
                [
                    data,
                    structure,
                    isa + f" (B={size})",
                    dtype,
                    int(size),
                    int(parts[3].split("/")[1].split("_")[0]),
                ]
            )
        return pd.Series(
            [
                data,
                parts[0],
                "STL",
                parts[1].split("/")[0],
                int(0),
                int(parts[1].split("/")[1].split("_")[0]),
            ]
        )

    if "json" in json_file:
        df[["Entry", "Structure", "ISA", "Type", "B", "N"]] = df["name"].apply(
            parse_name
        )
        df["ind"] = (
            df["Structure"]
            + df["ISA"]
            + df["Type"]
            + pd.Series([str(s) for s in df["B"]])
            + pd.Series([str(s) for s in df["N"]])
        )
    ind = ["Structure", "ISA", "Type", "B", "N"]
    baseline_df = None
    if "json" in json_file:
        df_pivot = df.pivot_table(
            index=ind,
            columns="Entry",
            values=["cpu_time"],
            aggfunc="mean",
        ).reset_index()
        df_pivot.columns = [
            f"cpu_{inde}" if not col in ind else col for col, inde in df_pivot.columns
        ]

        df_pivot.columns.name = None
        print(df_pivot)
        df = df_pivot
        # 2. Calculate Speedup
        # First, create a reference table of the baseline timings
        baseline_df = df[df["ISA"] == "STL"][["N", "cpu_mean", "Type"]]
        baseline_df = baseline_df.rename(columns={"cpu_mean": "baseline_time"})
    else:
        baseline_df = df[df["ISA"] == "STL"][["N", "cpu_mean", "Type"]]
        baseline_df = baseline_df.rename(columns={"cpu_mean": "baseline_time"})
    # Merge the baseline times back into the main dataframe
    df = df.merge(baseline_df, on=["N", "Type"], how="left")
    df["speedup"] = df["baseline_time"] / df["cpu_mean"]
    # 3. Setup Plotting Aesthetics
    sns.set_theme(style="whitegrid")

    for dt in df["Type"].unique():
        plt.figure(figsize=(12, 6))
        # 4. Create the Plot
        # We'll plot Size on X and Time on Y, using 'ISA' as the color hue
        plot = sns.linemarkerplot = sns.lineplot(
            data=df[df["Type"] == dt],
            x="N",
            y="speedup",
            hue="ISA",
            style="ISA",
            markers=True,
            dashes=False,
        )

        # Formatting
        plt.title("Benchmark Performance by ISA and Size", fontsize=15)
        plt.xlabel("Input Size (N)", fontsize=12)
        plt.ylabel(f"Speed up", fontsize=12)
        if "8" not in dt:
            plt.xscale("log")  # Often useful for benchmark sizes
        # plt.yscale("log")
        plt.legend(title="Configurations", loc="upper left")
        plt.tight_layout()

        # 5. Save/Show
        plt.savefig(f"{sys.argv[2]}/benchmark_results_{dt}.png", dpi=300)


if __name__ == "__main__":
    # Replace with your actual filename
    plot_benchmarks(sys.argv[1])
    pass
