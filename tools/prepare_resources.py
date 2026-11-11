"""Copy scene resources from local CHAI3D and Force Dimension SDK installs."""

import argparse
from pathlib import Path
import subprocess


def chai3d_asset(root, relative):
    path = root / relative
    if path.is_file():
        return path.read_bytes()
    # A sparse checkout may have the resource in Git but not in the worktree.
    if (root / ".git").exists():
        result = subprocess.run(
            ["git", "-C", str(root), "show", f"HEAD:{relative}"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode == 0:
            return result.stdout
    raise FileNotFoundError(f"CHAI3D resource missing: {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chai3d-root", type=Path, required=True)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument(
        "--output", type=Path,
        default=Path(__file__).resolve().parents[1] / "bin" / "resources",
    )
    args = parser.parse_args()

    images = [
        "blackstone.jpg", "brownboard.jpg", "whitefoam.jpg",
        "spheremap-2.jpg", "spheremap-3.jpg", "spheremap-4.jpg", "spheremap-5.jpg",
    ]
    resources = {
        f"images/{name}": chai3d_asset(args.chai3d_root, f"bin/resources/images/{name}")
        for name in images
    }
    for name in ["box.obj", "box.mtl"]:
        resources[f"models/box/{name}"] = chai3d_asset(
            args.chai3d_root, f"modules/GEL/bin/resources/models/box/{name}"
        )
    for name in ["micro.png", "sand.png", "palette.jpg", "paper.jpg", "shadow.png", "ground.jpg"]:
        source = args.sdk_root / "bin" / "resources" / "images" / name
        if not source.is_file():
            raise FileNotFoundError(f"Force Dimension SDK resource missing: {source}")
        resources[f"images/{name}"] = source.read_bytes()

    for relative, data in resources.items():
        destination = args.output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    print(f"Prepared {len(resources)} resources in {args.output.resolve()}")


if __name__ == "__main__":
    main()
