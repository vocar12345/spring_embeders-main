"""
batch_visualise.py
─────────────────────────────────────────────────────────────────────────────
Reads every subdirectory in Output/, finds nodes.csv and edges.csv,
and produces a publication-ready layout PDF for each graph.

Output:  Output/<graph_name>/layout.pdf

Requirements:  pip install matplotlib pandas numpy
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from pathlib import Path
import sys

# ── Config ────────────────────────────────────────────────────────────────────

OUTPUT_ROOT = Path("Output")

NODE_SIZE   = 120
NODE_COLOUR = "#2b6cb0"
NODE_EDGE_C = "white"
EDGE_COLOUR = "#90cdf4"
EDGE_ALPHA  = 0.6
EDGE_WIDTH  = 0.9
BG_COLOUR   = "#0d1117"
LABEL_COLOUR= "#e2e8f0"

plt.rcParams.update({
    "font.family"    : "serif",
    "font.size"      : 9,
    "savefig.dpi"    : 300,
    "savefig.bbox"   : "tight",
})

# ── Helpers ───────────────────────────────────────────────────────────────────

def render_graph(graph_dir: Path):
    nodes_csv = graph_dir / "nodes.csv"
    edges_csv = graph_dir / "edges.csv"

    if not nodes_csv.exists() or not edges_csv.exists():
        print(f"  [SKIP] Missing nodes.csv or edges.csv in {graph_dir}")
        return

    nodes = pd.read_csv(nodes_csv)
    edges = pd.read_csv(edges_csv)
    name  = graph_dir.name

    print(f"  Rendering: {name}  "
          f"(|V|={len(nodes)}  |E|={len(edges)}) ... ", end="")

    # ── Figure setup ──────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(10, 6), facecolor=BG_COLOUR)
    ax.set_facecolor(BG_COLOUR)

    # Fixed axis limits with margin
    margin = 60.0
    ax.set_xlim(nodes["x"].min() - margin, nodes["x"].max() + margin)
    ax.set_ylim(nodes["y"].min() - margin, nodes["y"].max() + margin)
    ax.set_aspect("equal")
    ax.set_xticks([]); ax.set_yticks([])
    for sp in ax.spines.values():
        sp.set_visible(False)

    # ── Draw edges (batched) ──────────────────────────────────
    pos = nodes.set_index("node_id")[["x", "y"]]
    xs, ys = [], []
    for _, e in edges.iterrows():
        s, t = int(e["source"]), int(e["target"])
        if s in pos.index and t in pos.index:
            xs += [pos.at[s, "x"], pos.at[t, "x"], None]
            ys += [pos.at[s, "y"], pos.at[t, "y"], None]

    if xs:
        ax.plot(xs, ys,
                color=EDGE_COLOUR, lw=EDGE_WIDTH,
                alpha=EDGE_ALPHA, zorder=1,
                solid_capstyle="round")

    # ── Draw nodes ────────────────────────────────────────────
    ax.scatter(nodes["x"], nodes["y"],
               s=NODE_SIZE, c=NODE_COLOUR,
               edgecolors=NODE_EDGE_C, linewidths=0.6, zorder=2)

    # ── Node ID labels ────────────────────────────────────────
    # Only label nodes if there are few enough to read clearly
    if len(nodes) <= 50:
        for _, row in nodes.iterrows():
            ax.text(row["x"], row["y"],
                    str(int(row["node_id"])),
                    ha="center", va="center",
                    fontsize=6, color="white",
                    fontweight="bold", zorder=3)

    # ── Title ─────────────────────────────────────────────────
    ax.set_title(
        f"{name}   |V| = {len(nodes)}   |E| = {len(edges)}",
        color="white", fontsize=11, pad=10
    )

    # ── Save ──────────────────────────────────────────────────
    out_path = graph_dir / "layout.pdf"
    plt.tight_layout()
    fig.savefig(out_path, facecolor=BG_COLOUR)
    plt.close(fig)
    print(f"saved -> {out_path}")


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if not OUTPUT_ROOT.exists():
        print(f"[ERROR] Output directory '{OUTPUT_ROOT}' not found.")
        print("  Run ./build/fr_batch first.")
        sys.exit(1)

    # Find all subdirectories that contain nodes.csv
    graph_dirs = sorted([
        d for d in OUTPUT_ROOT.iterdir()
        if d.is_dir() and (d / "nodes.csv").exists()
    ])

    if not graph_dirs:
        print(f"[ERROR] No processed graphs found in '{OUTPUT_ROOT}'.")
        print("  Run ./build/fr_batch first.")
        sys.exit(1)

    print(f"Found {len(graph_dirs)} graph(s) in '{OUTPUT_ROOT}/':\n")

    for graph_dir in graph_dirs:
        render_graph(graph_dir)

    print(f"\nAll done. PDFs saved alongside the CSV files in Output/.")


if __name__ == "__main__":
    main()