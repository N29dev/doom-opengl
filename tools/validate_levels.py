#!/usr/bin/env python3
"""Validate JSON level layouts without building the C++ game."""
import json
import sys
from collections import deque
from pathlib import Path

VALID = set("#.PDMHX")

def validate(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    rows = data["tiles"]
    assert 3 <= len(rows) <= 128, "height must be 3..128"
    assert isinstance(rows[0], str) and 3 <= len(rows[0]) <= 128, "invalid width"
    assert all(isinstance(row, str) and len(row) == len(rows[0]) for row in rows), "non-rectangular grid"
    assert all(set(row) <= VALID for row in rows), "unknown tile"
    assert sum(row.count("P") for row in rows) == 1, "must have one spawn"
    assert sum(row.count("X") for row in rows) == 1, "must have one exit"

    # A closed door can be opened during play, so consider D passable.
    start = next((x,y) for y,row in enumerate(rows) for x,tile in enumerate(row) if tile == "P")
    seen = {start}
    frontier = deque([start])
    while frontier:
        x,y = frontier.popleft()
        for nx,ny in [(x+1,y),(x-1,y),(x,y+1),(x,y-1)]:
            if (nx,ny) in seen or not 0 <= ny < len(rows) or not 0 <= nx < len(rows[0]):
                continue
            if rows[ny][nx] == "#":
                continue
            seen.add((nx,ny));frontier.append((nx,ny))
    assert any(rows[y][x] == "X" for x,y in seen), "exit unreachable from spawn"
    print(f"OK {path}: {len(rows[0])}x{len(rows)} tiles, reachable exit")

if __name__ == "__main__":
    paths = list(map(Path, sys.argv[1:])) or list(Path("assets/levels").glob("*.json"))
    if not paths:
        sys.exit("No levels found")
    for path in paths:
        validate(path)
