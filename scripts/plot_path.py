"""Plot the integrated fictive path from a FicTrac .dat file.

Takes an optional path argument: a .dat file to plot directly, or a
directory in which to look for the most recent *.dat. With no argument,
searches the current working directory. Converts the integrated x/y
(cols 15-16, radians) to millimetres using a 9 mm sphere, and shows an
interactive path plot coloured by elapsed time.
"""

from pathlib import Path

import click
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.collections import LineCollection

SPHERE_DIAMETER_MM = 9.0
SPHERE_RADIUS_MM = SPHERE_DIAMETER_MM / 2.0


def resolve_dat(target: Path) -> Path:
    if target.is_file():
        if target.suffix != ".dat":
            click.echo(f"Warning: {target.name} does not have a .dat extension.", err=True)
        return target
    candidates = sorted(target.glob("*.dat"), key=lambda p: p.stat().st_mtime)
    if not candidates:
        raise click.ClickException(f"No *.dat file found in {target}")
    chosen = candidates[-1]
    if len(candidates) > 1:
        click.echo(f"Found {len(candidates)} .dat files in {target}, using most recent: {chosen.name}")
    return chosen


def load(path: Path) -> pd.DataFrame:
    df = pd.read_csv(path, header=None)
    # FicTrac writes 25 cols per doc/data_header.txt
    cols_of_interest = {14: "int_x_rad", 15: "int_y_rad", 21: "t_ms"}
    df = df[list(cols_of_interest)].rename(columns=cols_of_interest)
    df["x_mm"] = df["int_x_rad"] * SPHERE_RADIUS_MM
    df["y_mm"] = df["int_y_rad"] * SPHERE_RADIUS_MM
    df["t_s"] = (df["t_ms"] - df["t_ms"].iloc[0]) / 1000.0
    return df


def plot(df: pd.DataFrame, title: str) -> None:
    # Swap X/Y so animal initial heading points up (+Y), per doc/coordinate_frames.md.
    px = df["y_mm"].to_numpy()
    py = df["x_mm"].to_numpy()
    t = df["t_s"].to_numpy()

    pts = np.column_stack([px, py]).reshape(-1, 1, 2)
    segs = np.concatenate([pts[:-1], pts[1:]], axis=1)

    fig, ax = plt.subplots(figsize=(7, 7))
    lc = LineCollection(segs, cmap="viridis", norm=plt.Normalize(t.min(), t.max()))
    lc.set_array(t[:-1])
    lc.set_linewidth(1.5)
    ax.add_collection(lc)

    ax.scatter(px[0], py[0], c="lime", s=60, edgecolor="black", zorder=3, label="start")
    ax.scatter(px[-1], py[-1], c="red", s=60, edgecolor="black", zorder=3, label="end")

    ax.set_xlabel("animal-right (mm)")
    ax.set_ylabel("animal-forward (mm)")
    ax.set_aspect("equal", adjustable="datalim")
    ax.autoscale_view()
    ax.grid(alpha=0.3)
    ax.legend(loc="best")
    ax.set_title(f"{title}\n{t[-1]:.1f} s, {len(df)} frames")

    cbar = fig.colorbar(lc, ax=ax, shrink=0.85)
    cbar.set_label("elapsed time (s)")

    plt.tight_layout()
    plt.show()


@click.command(help=__doc__)
@click.argument(
    "path",
    type=click.Path(exists=True, file_okay=True, dir_okay=True, path_type=Path),
    default=Path.cwd(),
    required=False,
)
def main(path: Path) -> None:
    dat = resolve_dat(path)
    df = load(dat)
    click.echo(f"Loaded {len(df)} frames from {dat.name}")
    plot(df, dat.name)


if __name__ == "__main__":
    main()
