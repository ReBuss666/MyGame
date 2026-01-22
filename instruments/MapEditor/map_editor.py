"""
- Shift + Right click (select mode): apply one texture to all walls of the clicked sector.
- Mouse wheel to zoom; Middle mouse drag to pan.

Export format (JSON):
{
  "sectors": [
    {
      "id": 0,
      "floorHeight": 0.0,
      "ceilingHeight": 3.0,
      "lightLevel": 255,
      "floorTexture": "",
      "ceilingTexture": "",
      "flags": 0,
      "walls": [
        {
          "start": [0.0, 0.0],
          "end": [4.0, 0.0],
          "neighborId": 1,
          "upperTexture": "",
          "middleTexture": "",
          "lowerTexture": "",
          "flags": 0
        }
      ]
    }
  ],
  "textures": ["wall.png", "cafel.png", "greywall.png", "bluewall.png]
}
"""
from __future__ import annotations

import json
import math
import os
from dataclasses import dataclass, asdict
from typing import List, Optional, Tuple
from pathlib import Path

from PySide6 import QtCore, QtGui, QtWidgets

# Default save directory (relative to this script)
SCRIPT_DIR = Path(__file__).parent
DEFAULT_MAP_DIR = SCRIPT_DIR.parent.parent / "assets" / "maps"

Point = Tuple[float, float]


def point_in_polygon(pt: Point, vertices: List[Point]) -> bool:
    """Ray casting point-in-polygon."""
    x, y = pt
    inside = False
    n = len(vertices)
    if n < 3:
        return False
    for i in range(n):
        x1, y1 = vertices[i]
        x2, y2 = vertices[(i + 1) % n]
        if ((y1 > y) != (y2 > y)):
            slope = (x2 - x1) / (y2 - y1)
            x_at_y = x1 + slope * (y - y1)
            if x < x_at_y:
                inside = not inside
    return inside


def light_to_color(level: int) -> QtGui.QColor:
    level = max(0, min(255, level))
    return QtGui.QColor(level, level, level, 80)


def wall_color(has_neighbor: bool) -> QtGui.QColor:
    return QtGui.QColor(30, 180, 30) if has_neighbor else QtGui.QColor(200, 60, 60)


def point_segment_distance(point: Point, a: Point, b: Point) -> float:
    """Return perpendicular distance from point to segment AB."""
    px, py = point
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    if dx == 0 and dy == 0:
        return ((px - ax) ** 2 + (py - ay) ** 2) ** 0.5
    t = ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)
    t = max(0.0, min(1.0, t))
    cx, cy = ax + t * dx, ay + t * dy
    return ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5


def snap_to_angle(origin: Point, target: Point) -> Point:
    """Snap target point to 0, 45, 90, 135, 180... degree angles from origin."""
    dx = target[0] - origin[0]
    dy = target[1] - origin[1]
    dist = math.hypot(dx, dy)
    if dist < 0.001:
        return target
    angle = math.atan2(dy, dx)
    # Snap to nearest 45 degrees (pi/4)
    snap_angle = round(angle / (math.pi / 4)) * (math.pi / 4)
    new_x = origin[0] + dist * math.cos(snap_angle)
    new_y = origin[1] + dist * math.sin(snap_angle)
    return (new_x, new_y)


@dataclass
class WallData:
    start: Point
    end: Point
    neighbor_id: Optional[int] = None
    upper_texture: str = ""
    middle_texture: str = ""
    lower_texture: str = ""
    flags: int = 0

    def to_dict(self) -> dict:
        return {
            "start": [self.start[0], self.start[1]],
            "end": [self.end[0], self.end[1]],
            "neighborId": self.neighbor_id,
            "upperTexture": self.upper_texture,
            "middleTexture": self.middle_texture,
            "lowerTexture": self.lower_texture,
            "flags": self.flags,
        }

    @staticmethod
    def from_dict(data: dict) -> "WallData":
        return WallData(
            start=tuple(data.get("start", (0.0, 0.0)))[:2],
            end=tuple(data.get("end", (0.0, 0.0)))[:2],
            neighbor_id=data.get("neighborId"),
            upper_texture=data.get("upperTexture", ""),
            middle_texture=data.get("middleTexture", ""),
            lower_texture=data.get("lowerTexture", ""),
            flags=data.get("flags", 0),
        )


@dataclass
class SectorData:
    sector_id: int
    floor_height: float = 0.0
    ceiling_height: float = 3.0
    light_level: int = 255
    floor_texture: str = ""
    ceiling_texture: str = ""
    flags: int = 0
    walls: List[WallData] = None

    def __post_init__(self):
        if self.walls is None:
            self.walls = []

    def to_dict(self) -> dict:
        return {
            "id": self.sector_id,
            "floorHeight": self.floor_height,
            "ceilingHeight": self.ceiling_height,
            "lightLevel": self.light_level,
            "floorTexture": self.floor_texture,
            "ceilingTexture": self.ceiling_texture,
            "flags": self.flags,
            "walls": [w.to_dict() for w in self.walls],
        }

    @staticmethod
    def from_dict(data: dict) -> "SectorData":
        walls = [WallData.from_dict(w) for w in data.get("walls", [])]
        return SectorData(
            sector_id=data.get("id", -1),
            floor_height=data.get("floorHeight", 0.0),
            ceiling_height=data.get("ceilingHeight", 3.0),
            light_level=data.get("lightLevel", 255),
            floor_texture=data.get("floorTexture", ""),
            ceiling_texture=data.get("ceilingTexture", ""),
            flags=data.get("flags", 0),
            walls=walls,
        )

    def vertices(self) -> List[Point]:
        return [w.start for w in self.walls]

    def contains(self, pt: Point) -> bool:
        return point_in_polygon(pt, self.vertices())


class MapScene(QtWidgets.QGraphicsScene):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSceneRect(-2000, -2000, 4000, 4000)


class MapView(QtWidgets.QGraphicsView):
    point_clicked = QtCore.Signal(QtCore.QPointF, QtCore.Qt.MouseButton)
    middle_dragged = QtCore.Signal(QtCore.QPointF)
    mouse_moved = QtCore.Signal(QtCore.QPointF, bool)  # pos, shift_held
    mouse_released = QtCore.Signal()

    def __init__(self, scene: QtWidgets.QGraphicsScene, parent=None):
        super().__init__(scene, parent)
        self.setRenderHint(QtGui.QPainter.Antialiasing)
        self.setDragMode(QtWidgets.QGraphicsView.NoDrag)
        self._panning = False
        self._pan_start = QtCore.QPoint()
        self.setMouseTracking(True)

    def mousePressEvent(self, event: QtGui.QMouseEvent):
        if event.button() == QtCore.Qt.MiddleButton:
            self._panning = True
            self._pan_start = event.pos()
            self.setCursor(QtCore.Qt.ClosedHandCursor)
            event.accept()
            return
        scene_pos = self.mapToScene(event.position().toPoint())
        self.point_clicked.emit(scene_pos, event.button())
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event: QtGui.QMouseEvent):
        if event.button() == QtCore.Qt.MiddleButton and self._panning:
            self._panning = False
            self.setCursor(QtCore.Qt.ArrowCursor)
            event.accept()
            return
        if event.button() == QtCore.Qt.LeftButton:
            self.mouse_released.emit()
        super().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event: QtGui.QMouseEvent):
        if self._panning:
            delta = event.pos() - self._pan_start
            self._pan_start = event.pos()
            # Use scroll bars for proper panning
            self.horizontalScrollBar().setValue(self.horizontalScrollBar().value() - delta.x())
            self.verticalScrollBar().setValue(self.verticalScrollBar().value() - delta.y())
            event.accept()
            return
        scene_pos = self.mapToScene(event.position().toPoint())
        shift_held = bool(event.modifiers() & QtCore.Qt.ShiftModifier)
        self.mouse_moved.emit(scene_pos, shift_held)
        super().mouseMoveEvent(event)

    def wheelEvent(self, event: QtGui.QWheelEvent):
        factor = 1.15 if event.angleDelta().y() > 0 else 1 / 1.15
        self.scale(factor, factor)


class WallPropertiesPanel(QtWidgets.QWidget):
    """Panel for editing individual wall properties."""
    changed = QtCore.Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._wall: Optional[WallData] = None
        self._wall_index: int = -1
        self._updating = False
        self._textures: List[str] = []
        self._build_ui()

    def _build_ui(self):
        layout = QtWidgets.QVBoxLayout(self)
        
        self.title_label = QtWidgets.QLabel("Wall Properties")
        self.title_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        layout.addWidget(self.title_label)

        form = QtWidgets.QFormLayout()
        
        # Textures with combo boxes
        self.upper_tex = QtWidgets.QComboBox()
        self.upper_tex.setEditable(True)
        self.middle_tex = QtWidgets.QComboBox()
        self.middle_tex.setEditable(True)
        self.lower_tex = QtWidgets.QComboBox()
        self.lower_tex.setEditable(True)
        
        self.neighbor_spin = QtWidgets.QSpinBox()
        self.neighbor_spin.setRange(-1, 9999)
        self.neighbor_spin.setSpecialValueText("None")
        
        # Wall flags
        self.flag_blocking = QtWidgets.QCheckBox("Blocking")
        self.flag_transparent = QtWidgets.QCheckBox("Transparent")
        self.flag_two_sided = QtWidgets.QCheckBox("Two-Sided")
        self.flag_door = QtWidgets.QCheckBox("Door")

        form.addRow("Upper Texture", self.upper_tex)
        form.addRow("Middle Texture", self.middle_tex)
        form.addRow("Lower Texture", self.lower_tex)
        form.addRow("Neighbor Sector", self.neighbor_spin)
        layout.addLayout(form)

        flags_layout = QtWidgets.QHBoxLayout()
        flags_layout.addWidget(self.flag_blocking)
        flags_layout.addWidget(self.flag_transparent)
        layout.addLayout(flags_layout)
        flags_layout2 = QtWidgets.QHBoxLayout()
        flags_layout2.addWidget(self.flag_two_sided)
        flags_layout2.addWidget(self.flag_door)
        layout.addLayout(flags_layout2)

        # Quick texture buttons
        quick_layout = QtWidgets.QHBoxLayout()
        self.btn_copy_to_all = QtWidgets.QPushButton("Copy to All Walls")
        self.btn_copy_to_all.clicked.connect(self._copy_to_all_walls)
        quick_layout.addWidget(self.btn_copy_to_all)
        layout.addLayout(quick_layout)

        # Connect signals
        for combo in [self.upper_tex, self.middle_tex, self.lower_tex]:
            combo.currentTextChanged.connect(self._emit_changed)
        self.neighbor_spin.valueChanged.connect(self._emit_changed)
        for cb in [self.flag_blocking, self.flag_transparent, self.flag_two_sided, self.flag_door]:
            cb.stateChanged.connect(self._emit_changed)

        layout.addStretch(1)

    def set_textures(self, textures: List[str]):
        self._textures = textures
        for combo in [self.upper_tex, self.middle_tex, self.lower_tex]:
            current = combo.currentText()
            combo.clear()
            combo.addItem("")
            combo.addItems(textures)
            combo.setCurrentText(current)

    def set_wall(self, wall: Optional[WallData], index: int = -1):
        self._wall = wall
        self._wall_index = index
        self._updating = True
        
        if not wall:
            self.title_label.setText("Wall Properties (none selected)")
            self.upper_tex.setCurrentText("")
            self.middle_tex.setCurrentText("")
            self.lower_tex.setCurrentText("")
            self.neighbor_spin.setValue(-1)
            for cb in [self.flag_blocking, self.flag_transparent, self.flag_two_sided, self.flag_door]:
                cb.setChecked(False)
            self._updating = False
            return

        self.title_label.setText(f"Wall {index} Properties")
        self.upper_tex.setCurrentText(wall.upper_texture)
        self.middle_tex.setCurrentText(wall.middle_texture)
        self.lower_tex.setCurrentText(wall.lower_texture)
        self.neighbor_spin.setValue(wall.neighbor_id if wall.neighbor_id is not None else -1)
        self.flag_blocking.setChecked(bool(wall.flags & 1))
        self.flag_transparent.setChecked(bool(wall.flags & 2))
        self.flag_two_sided.setChecked(bool(wall.flags & 4))
        self.flag_door.setChecked(bool(wall.flags & 8))
        self._updating = False

    def _emit_changed(self):
        if self._updating or not self._wall:
            return
        self.apply_to_wall()
        self.changed.emit()

    def apply_to_wall(self):
        if not self._wall:
            return
        self._wall.upper_texture = self.upper_tex.currentText()
        self._wall.middle_texture = self.middle_tex.currentText()
        self._wall.lower_texture = self.lower_tex.currentText()
        val = self.neighbor_spin.value()
        self._wall.neighbor_id = val if val >= 0 else None
        flags = 0
        if self.flag_blocking.isChecked(): flags |= 1
        if self.flag_transparent.isChecked(): flags |= 2
        if self.flag_two_sided.isChecked(): flags |= 4
        if self.flag_door.isChecked(): flags |= 8
        self._wall.flags = flags

    def _copy_to_all_walls(self):
        """Signal parent to copy current wall textures to all walls in sector."""
        self.changed.emit()  # Will be handled specially


class TextureManagerPanel(QtWidgets.QWidget):
    """Panel for managing texture list."""
    textures_changed = QtCore.Signal(list)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._textures: List[str] = []
        self._build_ui()

    def _build_ui(self):
        layout = QtWidgets.QVBoxLayout(self)
        
        label = QtWidgets.QLabel("Texture Library")
        label.setStyleSheet("font-weight: bold;")
        layout.addWidget(label)

        self.texture_list = QtWidgets.QListWidget()
        self.texture_list.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        layout.addWidget(self.texture_list, 1)

        btn_layout = QtWidgets.QHBoxLayout()
        self.btn_add = QtWidgets.QPushButton("+")
        self.btn_add.setMaximumWidth(30)
        self.btn_remove = QtWidgets.QPushButton("-")
        self.btn_remove.setMaximumWidth(30)
        btn_layout.addWidget(self.btn_add)
        btn_layout.addWidget(self.btn_remove)
        btn_layout.addStretch()
        layout.addLayout(btn_layout)

        self.btn_add.clicked.connect(self._add_texture)
        self.btn_remove.clicked.connect(self._remove_texture)

    def set_textures(self, textures: List[str]):
        self._textures = list(textures)
        self.texture_list.clear()
        self.texture_list.addItems(self._textures)

    def get_textures(self) -> List[str]:
        return self._textures

    def _add_texture(self):
        text, ok = QtWidgets.QInputDialog.getText(self, "Add Texture", "Texture name:")
        if ok and text and text not in self._textures:
            self._textures.append(text)
            self.texture_list.addItem(text)
            self.textures_changed.emit(self._textures)

    def _remove_texture(self):
        item = self.texture_list.currentItem()
        if item:
            self._textures.remove(item.text())
            self.texture_list.takeItem(self.texture_list.row(item))
            self.textures_changed.emit(self._textures)


class VertexEditorPanel(QtWidgets.QWidget):
    """Panel for editing sector vertices with manual coordinate input."""
    vertices_changed = QtCore.Signal()
    vertex_selected = QtCore.Signal(int)  # Emits selected vertex index (-1 if none)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._sector: Optional[SectorData] = None
        self._updating = False
        self._selected_vertex: int = -1
        self._build_ui()

    def _build_ui(self):
        layout = QtWidgets.QVBoxLayout(self)

        label = QtWidgets.QLabel("Vertices (Points)")
        label.setStyleSheet("font-weight: bold; font-size: 14px;")
        layout.addWidget(label)

        # Vertex table with editable X, Y columns
        self.vertex_table = QtWidgets.QTableWidget(0, 3)
        self.vertex_table.setHorizontalHeaderLabels(["#", "X", "Y"])
        self.vertex_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        self.vertex_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        self.vertex_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.vertex_table, 1)

        # Buttons for vertex manipulation
        btn_layout = QtWidgets.QHBoxLayout()
        self.btn_add_vertex = QtWidgets.QPushButton("+ Add")
        self.btn_delete_vertex = QtWidgets.QPushButton("- Delete")
        self.btn_move_up = QtWidgets.QPushButton("↑")
        self.btn_move_down = QtWidgets.QPushButton("↓")
        self.btn_move_up.setMaximumWidth(30)
        self.btn_move_down.setMaximumWidth(30)
        btn_layout.addWidget(self.btn_add_vertex)
        btn_layout.addWidget(self.btn_delete_vertex)
        btn_layout.addWidget(self.btn_move_up)
        btn_layout.addWidget(self.btn_move_down)
        layout.addLayout(btn_layout)

        # Quick input for new vertex
        input_layout = QtWidgets.QHBoxLayout()
        input_layout.addWidget(QtWidgets.QLabel("New:"))
        self.new_x = QtWidgets.QDoubleSpinBox()
        self.new_x.setRange(-10000, 10000)
        self.new_x.setDecimals(1)
        self.new_x.setPrefix("X: ")
        self.new_y = QtWidgets.QDoubleSpinBox()
        self.new_y.setRange(-10000, 10000)
        self.new_y.setDecimals(1)
        self.new_y.setPrefix("Y: ")
        input_layout.addWidget(self.new_x)
        input_layout.addWidget(self.new_y)
        layout.addLayout(input_layout)

        # Connect signals
        self.vertex_table.cellChanged.connect(self._on_cell_changed)
        self.vertex_table.itemSelectionChanged.connect(self._on_selection_changed)
        self.btn_add_vertex.clicked.connect(self._add_vertex)
        self.btn_delete_vertex.clicked.connect(self._delete_vertex)
        self.btn_move_up.clicked.connect(self._move_vertex_up)
        self.btn_move_down.clicked.connect(self._move_vertex_down)

        layout.addStretch(1)

    def get_selected_vertex(self) -> int:
        return self._selected_vertex

    def set_sector(self, sector: Optional[SectorData]):
        self._sector = sector
        self._selected_vertex = -1
        self._populate_table()

    def _on_selection_changed(self):
        row = self.vertex_table.currentRow()
        self._selected_vertex = row if row >= 0 else -1
        self.vertex_selected.emit(self._selected_vertex)

    def _populate_table(self):
        self._updating = True
        if not self._sector:
            self.vertex_table.setRowCount(0)
            self._updating = False
            return

        verts = self._sector.vertices()
        self.vertex_table.setRowCount(len(verts))

        for row, (x, y) in enumerate(verts):
            # Index column (read-only)
            idx_item = QtWidgets.QTableWidgetItem(str(row))
            idx_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
            idx_item.setTextAlignment(QtCore.Qt.AlignCenter)
            self.vertex_table.setItem(row, 0, idx_item)

            # X column (editable)
            x_item = QtWidgets.QTableWidgetItem(f"{x:.1f}")
            x_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled)
            self.vertex_table.setItem(row, 1, x_item)

            # Y column (editable)
            y_item = QtWidgets.QTableWidgetItem(f"{y:.1f}")
            y_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled)
            self.vertex_table.setItem(row, 2, y_item)

        self._updating = False

    def _on_cell_changed(self, row: int, column: int):
        if self._updating or not self._sector:
            return
        if column not in (1, 2):  # Only X and Y columns
            return

        walls = self._sector.walls
        if row < 0 or row >= len(walls):
            return

        try:
            val = float(self.vertex_table.item(row, column).text())
        except (TypeError, ValueError):
            self._populate_table()  # Revert on invalid input
            return

        # Update the vertex
        old_pos = walls[row].start
        if column == 1:  # X
            new_pos = (val, old_pos[1])
        else:  # Y
            new_pos = (old_pos[0], val)

        # Update this wall's start and previous wall's end
        walls[row].start = new_pos
        prev_idx = (row - 1) % len(walls)
        walls[prev_idx].end = new_pos

        self.vertices_changed.emit()

    def _add_vertex(self):
        if not self._sector:
            return

        x = self.new_x.value()
        y = self.new_y.value()
        walls = self._sector.walls

        if len(walls) == 0:
            # First vertex - create a degenerate wall
            walls.append(WallData(start=(x, y), end=(x, y)))
        else:
            # Insert new vertex after selected row (or at end)
            row = self.vertex_table.currentRow()
            if row < 0:
                row = len(walls) - 1

            # New wall from new point to next point
            next_idx = (row + 1) % len(walls)
            next_start = walls[next_idx].start

            # Update current wall's end to new point
            new_wall = WallData(start=(x, y), end=next_start)
            walls[row].end = (x, y)

            # Insert new wall
            walls.insert(row + 1, new_wall)

        self._populate_table()
        self.vertices_changed.emit()

    def _delete_vertex(self):
        if not self._sector:
            return

        row = self.vertex_table.currentRow()
        walls = self._sector.walls

        if row < 0 or len(walls) <= 3:
            QtWidgets.QMessageBox.warning(self, "Cannot delete", "Sector must have at least 3 vertices.")
            return

        # Connect previous wall to next wall's end
        prev_idx = (row - 1) % len(walls)
        next_idx = (row + 1) % len(walls)

        walls[prev_idx].end = walls[next_idx].start
        walls.pop(row)

        self._populate_table()
        self.vertices_changed.emit()

    def _move_vertex_up(self):
        """Swap vertex with previous one."""
        if not self._sector:
            return
        row = self.vertex_table.currentRow()
        walls = self._sector.walls
        if row <= 0 or row >= len(walls):
            return

        # Swap starts
        walls[row].start, walls[row - 1].start = walls[row - 1].start, walls[row].start
        # Fix ends
        walls[row - 1].end = walls[row].start
        prev_prev = (row - 2) % len(walls)
        walls[prev_prev].end = walls[row - 1].start

        self._populate_table()
        self.vertex_table.selectRow(row - 1)
        self.vertices_changed.emit()

    def _move_vertex_down(self):
        """Swap vertex with next one."""
        if not self._sector:
            return
        row = self.vertex_table.currentRow()
        walls = self._sector.walls
        if row < 0 or row >= len(walls) - 1:
            return

        # Swap starts
        walls[row].start, walls[row + 1].start = walls[row + 1].start, walls[row].start
        # Fix ends
        prev_idx = (row - 1) % len(walls)
        walls[prev_idx].end = walls[row].start
        walls[row].end = walls[row + 1].start

        self._populate_table()
        self.vertex_table.selectRow(row + 1)
        self.vertices_changed.emit()


class PropertiesPanel(QtWidgets.QWidget):
    changed = QtCore.Signal()
    neighbor_changed = QtCore.Signal(int, Optional[int])

    def __init__(self, parent=None):
        super().__init__(parent)
        self._sector: Optional[SectorData] = None
        self._updating = False
        self._build_ui()

    def _build_ui(self):
        layout = QtWidgets.QVBoxLayout(self)
        form = QtWidgets.QFormLayout()

        self.floor_spin = QtWidgets.QDoubleSpinBox()
        self.floor_spin.setRange(-1000, 1000)
        self.floor_spin.setDecimals(3)
        self.ceiling_spin = QtWidgets.QDoubleSpinBox()
        self.ceiling_spin.setRange(-1000, 1000)
        self.ceiling_spin.setDecimals(3)
        self.light_spin = QtWidgets.QSpinBox()
        self.light_spin.setRange(0, 255)

        self.floor_tex = QtWidgets.QLineEdit()
        self.ceiling_tex = QtWidgets.QLineEdit()

        self.flag_damage = QtWidgets.QCheckBox("Damage")
        self.flag_secret = QtWidgets.QCheckBox("Secret")
        self.flag_water = QtWidgets.QCheckBox("Water")
        self.flag_outdoor = QtWidgets.QCheckBox("Outdoor")

        form.addRow("Floor height", self.floor_spin)
        form.addRow("Ceiling height", self.ceiling_spin)
        form.addRow("Light (0-255)", self.light_spin)
        form.addRow("Floor texture", self.floor_tex)
        form.addRow("Ceiling texture", self.ceiling_tex)
        layout.addLayout(form)

        flags_layout = QtWidgets.QHBoxLayout()
        flags_layout.addWidget(self.flag_damage)
        flags_layout.addWidget(self.flag_secret)
        flags_layout.addWidget(self.flag_water)
        flags_layout.addWidget(self.flag_outdoor)
        layout.addLayout(flags_layout)

        self.wall_table = QtWidgets.QTableWidget(0, 3)
        self.wall_table.setHorizontalHeaderLabels(["Start", "End", "Neighbor ID (-1 none)"])
        self.wall_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        layout.addWidget(self.wall_table, 1)

        for widget in [
            self.floor_spin,
            self.ceiling_spin,
            self.light_spin,
            self.floor_tex,
            self.ceiling_tex,
            self.flag_damage,
            self.flag_secret,
            self.flag_water,
            self.flag_outdoor,
        ]:
            widget.editingFinished.connect(self._emit_changed) if hasattr(widget, "editingFinished") else widget.stateChanged.connect(self._emit_changed)

        self.wall_table.cellChanged.connect(self._wall_cell_changed)
        layout.addStretch(1)

    def set_sector(self, sector: Optional[SectorData]):
        self._sector = sector
        self._updating = True
        if not sector:
            self.floor_spin.setValue(0.0)
            self.ceiling_spin.setValue(3.0)
            self.light_spin.setValue(255)
            self.floor_tex.setText("")
            self.ceiling_tex.setText("")
            for cb in [self.flag_damage, self.flag_secret, self.flag_water, self.flag_outdoor]:
                cb.setChecked(False)
            self.wall_table.setRowCount(0)
            self._updating = False
            return
        self.floor_spin.setValue(sector.floor_height)
        self.ceiling_spin.setValue(sector.ceiling_height)
        self.light_spin.setValue(sector.light_level)
        self.floor_tex.setText(sector.floor_texture)
        self.ceiling_tex.setText(sector.ceiling_texture)
        self.flag_damage.setChecked(bool(sector.flags & 1))
        self.flag_secret.setChecked(bool(sector.flags & 2))
        self.flag_water.setChecked(bool(sector.flags & 4))
        self.flag_outdoor.setChecked(bool(sector.flags & 8))
        self._populate_walls()
        self._updating = False

    def _populate_walls(self):
        if not self._sector:
            self.wall_table.setRowCount(0)
            return
        self.wall_table.setRowCount(len(self._sector.walls))
        for row, wall in enumerate(self._sector.walls):
            start_item = QtWidgets.QTableWidgetItem(f"({wall.start[0]:.1f}, {wall.start[1]:.1f})")
            start_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
            end_item = QtWidgets.QTableWidgetItem(f"({wall.end[0]:.1f}, {wall.end[1]:.1f})")
            end_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled)
            neighbor_item = QtWidgets.QTableWidgetItem(str(wall.neighbor_id if wall.neighbor_id is not None else -1))
            neighbor_item.setFlags(QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled)
            self.wall_table.setItem(row, 0, start_item)
            self.wall_table.setItem(row, 1, end_item)
            self.wall_table.setItem(row, 2, neighbor_item)

    def _emit_changed(self):
        if self._updating:
            return
        self.changed.emit()

    def _wall_cell_changed(self, row: int, column: int):
        if self._updating or column != 2:
            return
        item = self.wall_table.item(row, column)
        try:
            val = int(item.text())
        except (TypeError, ValueError):
            val = -1
        neighbor = val if val >= 0 else None
        self.neighbor_changed.emit(row, neighbor)

    def apply_to_sector(self):
        if not self._sector:
            return
        self._sector.floor_height = self.floor_spin.value()
        self._sector.ceiling_height = self.ceiling_spin.value()
        self._sector.light_level = self.light_spin.value()
        self._sector.floor_texture = self.floor_tex.text()
        self._sector.ceiling_texture = self.ceiling_tex.text()
        flags = 0
        if self.flag_damage.isChecked():
            flags |= 1
        if self.flag_secret.isChecked():
            flags |= 2
        if self.flag_water.isChecked():
            flags |= 4
        if self.flag_outdoor.isChecked():
            flags |= 8
        self._sector.flags = flags
        # walls neighbors already updated via signal


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Sector Map Editor")
        self.scene = MapScene(self)
        self.view = MapView(self.scene, self)
        self.properties = PropertiesPanel(self)
        self.sectors: List[SectorData] = []
        self.current_vertices: List[Point] = []
        self.selected_sector: Optional[SectorData] = None
        self.selected_sectors: List[SectorData] = []  # Multi-selection with Alt
        self.selected_wall_index: Optional[int] = None
        self.mode_draw = False
        self.mode_grab = False
        self.mode_circle = False  # Circle drawing mode
        self.mode_line = False    # Line/rectangle drawing mode
        self.mode_marquee = False  # Marquee selection mode
        self.mode_split = False  # Split tool mode
        self.mode_stairs = False  # Stairs drawing mode
        self.grabbed_vertex_index: Optional[int] = None
        self.grab_origin: Optional[Point] = None  # For angle snapping reference
        self.grab_mode_type: str = "vertex"  # "vertex", "sector", "multi"
        self.grab_start_pos: Optional[Point] = None  # For sector/multi grab
        self.scale_mode = False  # For scaling with mouse wheel
        self.drag_start: Optional[Point] = None  # For circle/line drag drawing
        self.drag_current: Optional[Point] = None
        self.direct_vertex_drag: bool = False  # For dragging vertex in select mode
        self.split_points: List[Tuple[SectorData, int]] = []  # (sector, vertex_idx)
        self.multi_vertex_drag = False  # Dragging multiple selected vertices
        self.multi_vertex_offsets: List[Tuple[SectorData, int, float, float]] = []  # (sector, idx, dx, dy)
        self.snap_threshold = 15.0  # Snap to nearby vertices within this distance
        self.textures: List[str] = ["brick.png", "stone.png", "metal.png", "wood.png", "concrete.png"]
        self.selected_vertices: List[Tuple[SectorData, int]] = []  # List of (sector, vertex_index)
        self.circle_segments = 16  # Number of segments for circle
        self.marquee_start: Optional[Point] = None  # For marquee selection
        self.marquee_current: Optional[Point] = None
        self._build_ui()
        self._connect_signals()
        self._refresh_scene()

    # UI setup
    def _build_ui(self):
        splitter = QtWidgets.QSplitter()
        splitter.addWidget(self.view)
        
        # Right panel with tabs
        right_panel = QtWidgets.QTabWidget()
        self.properties = PropertiesPanel(self)
        self.wall_properties = WallPropertiesPanel(self)
        self.vertex_editor = VertexEditorPanel(self)
        self.texture_manager = TextureManagerPanel(self)
        
        right_panel.addTab(self.properties, "Sector")
        right_panel.addTab(self.wall_properties, "Wall")
        right_panel.addTab(self.vertex_editor, "Vertices")
        right_panel.addTab(self.texture_manager, "Textures")
        
        splitter.addWidget(right_panel)
        splitter.setStretchFactor(0, 3)
        splitter.setStretchFactor(1, 1)
        self.setCentralWidget(splitter)
        
        # Initialize texture lists
        self.texture_manager.set_textures(self.textures)
        self.wall_properties.set_textures(self.textures)

        toolbar = self.addToolBar("Tools")
        self.action_select = toolbar.addAction("Select")
        self.action_select.setShortcut(QtGui.QKeySequence("S"))
        self.action_draw = toolbar.addAction("Draw")
        self.action_draw.setShortcut(QtGui.QKeySequence("D"))
        self.action_circle = toolbar.addAction("Circle")
        self.action_circle.setShortcut(QtGui.QKeySequence("C"))
        self.action_line = toolbar.addAction("Line")
        self.action_line.setShortcut(QtGui.QKeySequence("L"))
        self.action_marquee = toolbar.addAction("Marquee")
        self.action_marquee.setShortcut(QtGui.QKeySequence("M"))
        toolbar.addSeparator()
        # Circle segments control
        toolbar.addWidget(QtWidgets.QLabel(" Segments: "))
        self.circle_segments_spin = QtWidgets.QSpinBox()
        self.circle_segments_spin.setRange(3, 10000)
        self.circle_segments_spin.setValue(16)
        self.circle_segments_spin.setFixedWidth(70)
        toolbar.addWidget(self.circle_segments_spin)
        toolbar.addSeparator()
        # Stairs controls
        toolbar.addWidget(QtWidgets.QLabel(" Steps: "))
        self.stairs_steps_spin = QtWidgets.QSpinBox()
        self.stairs_steps_spin.setRange(2, 20)
        self.stairs_steps_spin.setValue(5)
        self.stairs_steps_spin.setFixedWidth(70)
        toolbar.addWidget(self.stairs_steps_spin)
        toolbar.addWidget(QtWidgets.QLabel(" Height: "))
        self.step_height_spin = QtWidgets.QDoubleSpinBox()
        self.step_height_spin.setRange(0.1, 2.0)
        self.step_height_spin.setDecimals(1)
        self.step_height_spin.setValue(0.2)
        self.step_height_spin.setFixedWidth(70)
        toolbar.addWidget(self.step_height_spin)
        toolbar.addSeparator()
        self.action_grab = toolbar.addAction("Grab")
        self.action_grab.setShortcut(QtGui.QKeySequence("G"))
        self.action_scale = toolbar.addAction("Scale")
        self.action_scale.setShortcut(QtGui.QKeySequence("R"))
        toolbar.addSeparator()
        self.action_delete = toolbar.addAction("Delete")
        self.action_delete.setShortcut(QtGui.QKeySequence.StandardKey.Delete)
        toolbar.addSeparator()
        self.action_new = toolbar.addAction("New")
        self.action_new.setShortcut(QtGui.QKeySequence.StandardKey.New)
        self.action_load = toolbar.addAction("Load")
        self.action_load.setShortcut(QtGui.QKeySequence.StandardKey.Open)
        self.action_save = toolbar.addAction("Save")
        self.action_save.setShortcut(QtGui.QKeySequence.StandardKey.Save)
        self.action_select.setCheckable(True)
        self.action_draw.setCheckable(True)
        self.action_circle.setCheckable(True)
        self.action_line.setCheckable(True)
        self.action_marquee.setCheckable(True)
        self.action_grab.setCheckable(True)
        self.action_scale.setCheckable(True)
        self.action_split = toolbar.addAction("Split")
        self.action_split.setShortcut(QtGui.QKeySequence("P"))
        self.action_split.setCheckable(True)
        self.action_stairs = toolbar.addAction("Stairs")
        self.action_stairs.setShortcut(QtGui.QKeySequence("T"))
        self.action_stairs.setCheckable(True)
        self._set_mode_draw(False)

    def _connect_signals(self):
        self.view.point_clicked.connect(self._on_point_clicked)
        self.view.mouse_moved.connect(self._on_mouse_moved)
        self.view.mouse_released.connect(self._on_mouse_released)
        self.properties.changed.connect(self._on_properties_changed)
        self.properties.neighbor_changed.connect(self._on_neighbor_changed)
        self.wall_properties.changed.connect(self._on_wall_properties_changed)
        self.vertex_editor.vertices_changed.connect(self._on_vertices_changed)
        self.vertex_editor.vertex_selected.connect(self._on_vertex_selected)
        self.texture_manager.textures_changed.connect(self._on_textures_changed)
        self.action_select.triggered.connect(lambda: self._set_mode("select"))
        self.action_draw.triggered.connect(lambda: self._set_mode("draw"))
        self.action_circle.triggered.connect(lambda: self._set_mode("circle"))
        self.action_line.triggered.connect(lambda: self._set_mode("line"))
        self.action_marquee.triggered.connect(lambda: self._set_mode("marquee"))
        self.circle_segments_spin.valueChanged.connect(self._on_circle_segments_changed)
        self.action_grab.triggered.connect(lambda: self._set_mode("grab"))
        self.action_scale.triggered.connect(lambda: self._set_mode("scale"))
        self.action_delete.triggered.connect(self._delete_selected_sector)
        self.action_new.triggered.connect(self._new_map)
        self.action_load.triggered.connect(self._load_map)
        self.action_save.triggered.connect(self._save_map)
        self.action_split.triggered.connect(lambda: self._set_mode("split"))
        self.action_stairs.triggered.connect(lambda: self._set_mode("stairs"))

    # Mode handling
    def _set_mode(self, mode: str):
        self.mode_draw = (mode == "draw")
        self.mode_grab = (mode == "grab")
        self.mode_circle = (mode == "circle")
        self.mode_line = (mode == "line")
        self.mode_marquee = (mode == "marquee")
        self.mode_split = (mode == "split")
        self.mode_stairs = (mode == "stairs")
        self.scale_mode = (mode == "scale")
        self.action_draw.setChecked(self.mode_draw)
        self.action_select.setChecked(mode == "select")
        self.action_circle.setChecked(self.mode_circle)
        self.action_line.setChecked(self.mode_line)
        self.action_marquee.setChecked(self.mode_marquee)
        self.action_split.setChecked(self.mode_split)
        self.action_stairs.setChecked(self.mode_stairs)
        self.action_grab.setChecked(self.mode_grab)
        self.action_scale.setChecked(self.scale_mode)
        if not self.mode_draw:
            self.current_vertices.clear()
        if not self.mode_grab and not self.scale_mode:
            self.grabbed_vertex_index = None
            self.grab_origin = None
            self.grab_mode_type = "vertex"
            self.grab_start_pos = None
        self.drag_start = None
        self.drag_current = None
        self.direct_vertex_drag = False
        self.split_points.clear()
        self._refresh_scene()

    def _set_mode_draw(self, draw: bool):
        self._set_mode("draw" if draw else "select")

    # Event handlers
    def _on_point_clicked(self, point: QtCore.QPointF, button: QtCore.Qt.MouseButton):
        pos = (point.x(), point.y())
        modifiers = QtWidgets.QApplication.keyboardModifiers()
        alt_held = bool(modifiers & QtCore.Qt.AltModifier)
        ctrl_held = bool(modifiers & QtCore.Qt.ControlModifier)
        
        if self.mode_draw:
            if button == QtCore.Qt.LeftButton:
                self.current_vertices.append(pos)
            elif button == QtCore.Qt.RightButton:
                if self.current_vertices:
                    self._finish_sector()
                else:
                    self._delete_sector_at(pos)
            self._refresh_scene()
        elif self.mode_circle or self.mode_line:
            if button == QtCore.Qt.LeftButton:
                self.drag_start = pos
                self.drag_current = pos
            self._refresh_scene()
        elif self.mode_stairs:
            if button == QtCore.Qt.LeftButton:
                self.drag_start = pos
                self.drag_current = pos
            self._refresh_scene()
        elif self.mode_marquee:
            if button == QtCore.Qt.LeftButton:
                # Start marquee selection
                if not alt_held:
                    self.selected_vertices.clear()  # Clear previous selection unless Alt held
                self.marquee_start = pos
                self.marquee_current = pos
            elif button == QtCore.Qt.RightButton:
                # Delete selected vertices
                self._delete_selected_vertices()
            self._refresh_scene()
        elif self.mode_grab or self.scale_mode:
            if button == QtCore.Qt.LeftButton:
                # If there are selected vertices, start multi-vertex drag
                if self.selected_vertices and self.mode_grab:
                    self._start_multi_vertex_grab(pos)
                else:
                    self._start_grab(pos)
        elif self.mode_split:
            if button == QtCore.Qt.LeftButton:
                vertex_hit = self._find_vertex_at(pos)
                if vertex_hit:
                    if vertex_hit not in self.split_points:
                        self.split_points.append(vertex_hit)
                        if len(self.split_points) == 2:
                            # Inline split logic to avoid runtime method lookup issues
                            v1 = self.split_points[0]
                            v2 = self.split_points[1]
                            sector, idx1 = v1
                            _, idx2 = v2
                            if sector is not None and idx1 != idx2 and len(sector.walls) >= 4:
                                n = len(sector.walls)
                                path1 = []
                                i = idx1
                                while True:
                                    path1.append(i)
                                    if i == idx2:
                                        break
                                    i = (i + 1) % n
                                path2 = []
                                i = idx2
                                while True:
                                    path2.append(i)
                                    if i == idx1:
                                        break
                                    i = (i + 1) % n
                                verts1 = [sector.walls[i].start for i in path1]
                                verts2 = [sector.walls[i].start for i in path2]
                                if len(verts1) >= 3 and len(verts2) >= 3:
                                    portal1 = WallData(start=verts1[-1], end=verts1[0], neighbor_id=None)
                                    portal2 = WallData(start=verts2[-1], end=verts2[0], neighbor_id=None)
                                    walls1 = [WallData(start=verts1[i], end=verts1[(i+1)%len(verts1)]) for i in range(len(verts1)-1)] + [portal1]
                                    walls2 = [WallData(start=verts2[i], end=verts2[(i+1)%len(verts2)]) for i in range(len(verts2)-1)] + [portal2]
                                    if sector in self.sectors:
                                        self.sectors.remove(sector)
                                    new_id1 = max([s.sector_id for s in self.sectors]+[0])+1
                                    new_id2 = new_id1+1
                                    s1 = SectorData(sector_id=new_id1, walls=walls1, floor_height=sector.floor_height, ceiling_height=sector.ceiling_height, light_level=sector.light_level, floor_texture=sector.floor_texture, ceiling_texture=sector.ceiling_texture, flags=sector.flags)
                                    s2 = SectorData(sector_id=new_id2, walls=walls2, floor_height=sector.floor_height, ceiling_height=sector.ceiling_height, light_level=sector.light_level, floor_texture=sector.floor_texture, ceiling_texture=sector.ceiling_texture, flags=sector.flags)
                                    s1.walls[-1].neighbor_id = new_id2
                                    s2.walls[-1].neighbor_id = new_id1
                                    self.sectors.append(s1)
                                    self.sectors.append(s2)
                                    self._select_sector(None)
                                    self._refresh_scene()
                            self.split_points.clear()
            self._refresh_scene()
        else:
            # Select mode - allow direct vertex dragging
            if button == QtCore.Qt.LeftButton:
                # Check if clicking on a vertex first for direct drag
                vertex_hit = self._find_vertex_at(pos)
                if vertex_hit:
                    sector, v_idx = vertex_hit
                    self._select_sector(sector)
                    self.direct_vertex_drag = True
                    self.grabbed_vertex_index = v_idx
                    self.grab_origin = sector.walls[v_idx].start
                    self._refresh_scene()
                    return
                
                # Alt + click on edge to insert vertex
                if alt_held:
                    edge_hit = self._find_edge_at(pos)
                    if edge_hit:
                        sector, edge_idx = edge_hit
                        self._insert_vertex_at_edge(sector, edge_idx, pos)
                        return
                    # Alt + click for multi-selection
                    self._toggle_sector_selection(pos)
                else:
                    if self._try_wall_texture_shortcut(point, button):
                        return
                    # Try to select a wall first (click near wall line)
                    wall_selected = self._try_select_wall(pos)
                    if not wall_selected:
                        self._select_sector_at(pos)

    def _on_mouse_moved(self, point: QtCore.QPointF, shift_held: bool):
        pos = (point.x(), point.y())
        
        if self.mode_circle or self.mode_line:
            if self.drag_start:
                if shift_held:
                    # Snap to angle (45 degree increments)
                    pos = snap_to_angle(self.drag_start, pos)
                self.drag_current = pos
                self._refresh_scene()
        elif self.mode_stairs:
            if self.drag_start:
                if shift_held:
                    pos = snap_to_angle(self.drag_start, pos)
                self.drag_current = pos
                self._refresh_scene()
        elif self.mode_marquee and self.marquee_start:
            self.marquee_current = pos
            self._refresh_scene()
        elif self.direct_vertex_drag and self.grabbed_vertex_index is not None and self.selected_sector:
            # Direct vertex drag in select mode
            if shift_held and self.grab_origin:
                pos = snap_to_angle(self.grab_origin, pos)
            # Apply snapping to nearby vertices
            pos = self._snap_to_nearby_vertex(pos, exclude_sector=self.selected_sector, exclude_idx=self.grabbed_vertex_index)
            self._move_vertex(self.grabbed_vertex_index, pos)
            self._refresh_scene()
        elif self.scale_mode and self.grab_start_pos:
            # Scale mode - mouse Y distance determines scale factor
            self._handle_scale_drag(pos, shift_held)
            self._refresh_scene()
        elif self.mode_grab:
            if self.multi_vertex_drag and self.multi_vertex_offsets:
                # Dragging multiple selected vertices
                self._handle_multi_vertex_drag(pos, shift_held)
                self._refresh_scene()
            elif self.grab_mode_type == "vertex" and self.grabbed_vertex_index is not None and self.selected_sector:
                if shift_held and self.grab_origin:
                    pos = snap_to_angle(self.grab_origin, pos)
                # Apply snapping
                pos = self._snap_to_nearby_vertex(pos)
                self._move_vertex(self.grabbed_vertex_index, pos)
                self._refresh_scene()
            elif self.grab_mode_type in ("sector", "multi") and self.grab_start_pos:
                self._handle_sector_drag(pos, shift_held)
                self._refresh_scene()

    def _on_mouse_released(self):
        if self.mode_circle and self.drag_start and self.drag_current:
            self._create_circle_sector()
            self.drag_start = None
            self.drag_current = None
            self._refresh_scene()
        elif self.mode_line and self.drag_start and self.drag_current:
            self._create_line_sector()
            self.drag_start = None
            self.drag_current = None
            self._refresh_scene()
        elif self.mode_stairs and self.drag_start and self.drag_current:
            self._create_stairs_sector()
            self.drag_start = None
            self.drag_current = None
            self._refresh_scene()
        elif self.mode_marquee and self.marquee_start and self.marquee_current:
            self._finalize_marquee_selection()
            self.marquee_start = None
            self.marquee_current = None
            self._refresh_scene()
        elif self.direct_vertex_drag:
            self.direct_vertex_drag = False
            self.grabbed_vertex_index = None
            self.grab_origin = None
        elif self.mode_grab or self.scale_mode:
            self.grabbed_vertex_index = None
            self.grab_origin = None
            self.grab_mode_type = "vertex"
            self.grab_start_pos = None
            self._stored_positions = None
            self.multi_vertex_drag = False
            self.multi_vertex_offsets = []

    def _finish_sector(self):
        if len(self.current_vertices) < 3:
            QtWidgets.QMessageBox.warning(self, "Need more points", "Add at least 3 points before closing sector.")
            return
        next_id = 0 if not self.sectors else max(s.sector_id for s in self.sectors) + 1
        walls: List[WallData] = []
        verts = self.current_vertices
        for i in range(len(verts)):
            start = verts[i]
            end = verts[(i + 1) % len(verts)]
            walls.append(WallData(start=start, end=end))
        sector = SectorData(sector_id=next_id, walls=walls)
        self.sectors.append(sector)
        self.current_vertices = []
        self._select_sector(sector)
        self._refresh_scene()

    def _create_circle_sector(self):
        """Create a circular sector (polygon approximation) from drag start to current."""
        if not self.drag_start or not self.drag_current:
            return
        cx, cy = self.drag_start
        ex, ey = self.drag_current
        radius = math.sqrt((ex - cx) ** 2 + (ey - cy) ** 2)
        if radius < 10:  # Minimum size
            return
        
        # Create a polygon with configurable number of vertices
        num_vertices = self.circle_segments
        vertices = []
        for i in range(num_vertices):
            angle = 2 * math.pi * i / num_vertices
            x = cx + radius * math.cos(angle)
            y = cy + radius * math.sin(angle)
            vertices.append((x, y))
        
        next_id = 0 if not self.sectors else max(s.sector_id for s in self.sectors) + 1
        walls = []
        for i in range(len(vertices)):
            start = vertices[i]
            end = vertices[(i + 1) % len(vertices)]
            walls.append(WallData(start=start, end=end))
        
        sector = SectorData(sector_id=next_id, walls=walls)
        self.sectors.append(sector)
        self._select_sector(sector)

    def _create_line_sector(self):
        """Create a rectangular sector (line with thickness) from drag."""
        if not self.drag_start or not self.drag_current:
            return
        x1, y1 = self.drag_start
        x2, y2 = self.drag_current
        
        length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)
        if length < 10:  # Minimum size
            return
        
        # Line thickness (can be adjusted)
        thickness = 32.0
        
        # Calculate perpendicular offset
        dx = x2 - x1
        dy = y2 - y1
        # Normalize and rotate 90 degrees
        px = -dy / length * (thickness / 2)
        py = dx / length * (thickness / 2)
        
        # Create rectangle vertices
        vertices = [
            (x1 + px, y1 + py),  # Top-left
            (x1 - px, y1 - py),  # Bottom-left
            (x2 - px, y2 - py),  # Bottom-right
            (x2 + px, y2 + py),  # Top-right
        ]
        
        next_id = 0 if not self.sectors else max(s.sector_id for s in self.sectors) + 1
        walls = []
        for i in range(len(vertices)):
            start = vertices[i]
            end = vertices[(i + 1) % len(vertices)]
            walls.append(WallData(start=start, end=end))
        
        sector = SectorData(sector_id=next_id, walls=walls)
        self.sectors.append(sector)
        self._select_sector(sector)

    def _create_stairs_sector(self):
        """Create a series of rectangular sectors as stairs along the drag line."""
        if not self.drag_start or not self.drag_current:
            return
        x1, y1 = self.drag_start
        x2, y2 = self.drag_current
        
        length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)
        if length < 10:  # Minimum size
            return
        
        # Parameters
        thickness = 16.0
        num_steps = self.stairs_steps_spin.value()
        step_height = self.step_height_spin.value()
        
        # Calculate perpendicular offset
        dx = x2 - x1
        dy = y2 - y1
        px = -dy / length * (thickness / 2)
        py = dx / length * (thickness / 2)
        
        next_id = 0 if not self.sectors else max(s.sector_id for s in self.sectors) + 1
        
        for i in range(num_steps):
            # Position along the line
            t1 = i / num_steps
            t2 = (i + 1) / num_steps
            start_x = x1 + t1 * dx
            start_y = y1 + t1 * dy
            end_x = x1 + t2 * dx
            end_y = y1 + t2 * dy
            
            # Vertices for this step
            vertices = [
                (start_x + px, start_y + py),
                (start_x - px, start_y - py),
                (end_x - px, end_y - py),
                (end_x + px, end_y + py),
            ]
            
            walls = []
            for j in range(len(vertices)):
                wall = WallData(start=vertices[j], end=vertices[(j + 1) % len(vertices)])
                wall.middle_texture = self.textures[2] if len(self.textures) > 2 else self.textures[0] if self.textures else ""
                if j == 1 or j == 3:  # Side walls
                    wall.flags = 1  # Blocking
                walls.append(wall)
            
            floor_h = i * step_height
            sector = SectorData(
                sector_id=next_id + i, 
                walls=walls, 
                floor_height=floor_h,
                ceiling_height=5.0,
                floor_texture=self.textures[0] if self.textures else "",
                ceiling_texture=self.textures[1] if len(self.textures) > 1 else ""
            )
            self.sectors.append(sector)
            
            # Set neighbors: connect to previous and next step
            if i > 0:
                sector.walls[0].neighbor_id = next_id + i - 1  # Wall 0 is the "start" wall, connects to previous
            if i < num_steps - 1:
                sector.walls[2].neighbor_id = next_id + i + 1  # Wall 2 is the "end" wall, connects to next
        
        self._select_sector(self.sectors[-1])

    def _find_vertex_at(self, pos: Point, threshold: float = 15.0):
        """Find a vertex near the given position. Returns (sector, vertex_index) or None."""
        px, py = pos
        for sector in reversed(self.sectors):
            for i, wall in enumerate(sector.walls):
                vx, vy = wall.start
                dist = math.sqrt((px - vx) ** 2 + (py - vy) ** 2)
                if dist <= threshold:
                    return (sector, i)
        return None

    def _find_edge_at(self, pos: Point, threshold: float = 10.0):
        """Find an edge near the given position. Returns (sector, edge_index) or None."""
        px, py = pos
        for sector in reversed(self.sectors):
            for i, wall in enumerate(sector.walls):
                x1, y1 = wall.start
                x2, y2 = wall.end
                # Calculate distance from point to line segment
                dist = self._point_to_segment_distance(px, py, x1, y1, x2, y2)
                if dist <= threshold:
                    return (sector, i)
        return None

    def _point_to_segment_distance(self, px, py, x1, y1, x2, y2):
        """Calculate the shortest distance from point (px,py) to line segment (x1,y1)-(x2,y2)."""
        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy
        if length_sq == 0:
            return math.sqrt((px - x1) ** 2 + (py - y1) ** 2)
        
        t = max(0, min(1, ((px - x1) * dx + (py - y1) * dy) / length_sq))
        proj_x = x1 + t * dx
        proj_y = y1 + t * dy
        return math.sqrt((px - proj_x) ** 2 + (py - proj_y) ** 2)

    def _insert_vertex_at_edge(self, sector: SectorData, edge_idx: int, pos: Point):
        """Insert a new vertex at the given position on the specified edge."""
        wall = sector.walls[edge_idx]
        x1, y1 = wall.start
        x2, y2 = wall.end
        
        # Find the closest point on the edge
        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy
        if length_sq == 0:
            new_pos = (x1, y1)
        else:
            t = max(0, min(1, ((pos[0] - x1) * dx + (pos[1] - y1) * dy) / length_sq))
            new_pos = (x1 + t * dx, y1 + t * dy)
        
        # Create new wall from new_pos to old end
        new_wall = WallData(start=new_pos, end=wall.end)
        # Update current wall to end at new_pos
        wall.end = new_pos
        
        # Insert new wall after current wall
        sector.walls.insert(edge_idx + 1, new_wall)
        
        self._select_sector(sector)
        self._refresh_scene()

    def _on_circle_segments_changed(self, value: int):
        """Update the number of segments for circle drawing."""
        self.circle_segments = value

    def _finalize_marquee_selection(self):
        """Select all vertices within the marquee rectangle."""
        if not self.marquee_start or not self.marquee_current:
            return
        
        x1, y1 = self.marquee_start
        x2, y2 = self.marquee_current
        min_x, max_x = min(x1, x2), max(x1, x2)
        min_y, max_y = min(y1, y2), max(y1, y2)
        
        # Find all vertices within the rectangle
        for sector in self.sectors:
            for i, wall in enumerate(sector.walls):
                vx, vy = wall.start
                if min_x <= vx <= max_x and min_y <= vy <= max_y:
                    vertex_tuple = (sector, i)
                    if vertex_tuple not in self.selected_vertices:
                        self.selected_vertices.append(vertex_tuple)

    def _delete_selected_vertices(self):
        """Delete all vertices that are currently selected via marquee."""
        if not self.selected_vertices:
            return
        
        # Group vertices by sector using sector_id as key
        sector_vertices: dict = {}  # sector_id -> (sector, [indices])
        for sector, v_idx in self.selected_vertices:
            sid = id(sector)  # Use object id as key
            if sid not in sector_vertices:
                sector_vertices[sid] = (sector, [])
            sector_vertices[sid][1].append(v_idx)
        
        # Delete vertices from each sector (in reverse order to maintain indices)
        sectors_to_remove = []
        for sid, (sector, indices) in sector_vertices.items():
            # Sort indices in reverse order
            indices.sort(reverse=True)
            for v_idx in indices:
                if len(sector.walls) > 3:  # Keep at least 3 vertices
                    self._remove_vertex_from_sector(sector, v_idx)
                elif len(sector.walls) <= 3:
                    # Mark sector for removal if too few vertices remain
                    if sector not in sectors_to_remove:
                        sectors_to_remove.append(sector)
        
        # Remove sectors with too few vertices
        for sector in sectors_to_remove:
            if sector in self.sectors:
                self.sectors.remove(sector)
                if sector == self.selected_sector:
                    self._select_sector(None)
        
        self.selected_vertices.clear()
        self._refresh_scene()

    def _remove_vertex_from_sector(self, sector: SectorData, v_idx: int):
        """Remove a vertex from a sector by merging two walls."""
        if len(sector.walls) <= 3:
            return  # Can't have less than 3 vertices
        
        # Get the wall that starts at this vertex
        wall_to_remove = sector.walls[v_idx]
        # Get the previous wall (ends at this vertex)
        prev_idx = (v_idx - 1) % len(sector.walls)
        prev_wall = sector.walls[prev_idx]
        
        # Update previous wall to end at where removed wall ends
        prev_wall.end = wall_to_remove.end
        
        # Remove the wall
        sector.walls.pop(v_idx)

    def _start_multi_vertex_grab(self, pos: Point):
        """Start dragging multiple selected vertices together."""
        if not self.selected_vertices:
            return
        
        self.multi_vertex_drag = True
        self.grab_start_pos = pos
        self.multi_vertex_offsets = []
        
        # Store offsets from click position for each selected vertex
        for sector, v_idx in self.selected_vertices:
            if v_idx < len(sector.walls):
                vx, vy = sector.walls[v_idx].start
                dx = vx - pos[0]
                dy = vy - pos[1]
                self.multi_vertex_offsets.append((sector, v_idx, dx, dy))

    def _handle_multi_vertex_drag(self, pos: Point, shift_held: bool):
        """Move all selected vertices together."""
        if not self.multi_vertex_offsets:
            return
        
        # Apply snapping if close to another vertex
        snapped_pos = self._snap_to_nearby_vertex(pos, exclude_selected=True)
        
        # Calculate delta from snap
        snap_dx = snapped_pos[0] - pos[0]
        snap_dy = snapped_pos[1] - pos[1]
        
        for sector, v_idx, dx, dy in self.multi_vertex_offsets:
            if v_idx < len(sector.walls):
                new_x = pos[0] + dx + snap_dx
                new_y = pos[1] + dy + snap_dy
                new_pos = (new_x, new_y)
                
                # Update this vertex
                sector.walls[v_idx].start = new_pos
                # Update previous wall's end
                prev_idx = (v_idx - 1) % len(sector.walls)
                sector.walls[prev_idx].end = new_pos

    def _snap_to_nearby_vertex(self, pos: Point, exclude_sector: SectorData = None, 
                                exclude_idx: int = None, exclude_selected: bool = False) -> Point:
        """Snap position to nearby vertex or wall if within threshold."""
        px, py = pos
        best_dist = self.snap_threshold
        best_pos = pos
        
        # First check vertices
        for sector in self.sectors:
            for i, wall in enumerate(sector.walls):
                # Skip excluded vertex
                if exclude_sector and sector == exclude_sector and i == exclude_idx:
                    continue
                # Skip selected vertices if requested
                if exclude_selected and (sector, i) in self.selected_vertices:
                    continue
                
                vx, vy = wall.start
                dist = math.sqrt((px - vx) ** 2 + (py - vy) ** 2)
                if dist < best_dist:
                    best_dist = dist
                    best_pos = (vx, vy)
        
        # Then check walls (edges) - snap to closest point on wall
        for sector in self.sectors:
            for i, wall in enumerate(sector.walls):
                # Skip walls that belong to excluded vertex
                if exclude_sector and sector == exclude_sector:
                    # Skip wall starting from excluded vertex or previous wall
                    if i == exclude_idx:
                        continue
                    prev_idx = (exclude_idx - 1) % len(sector.walls)
                    if i == prev_idx:
                        continue
                
                x1, y1 = wall.start
                x2, y2 = wall.end
                
                # Find closest point on wall segment
                closest = self._closest_point_on_segment(px, py, x1, y1, x2, y2)
                dist = math.sqrt((px - closest[0]) ** 2 + (py - closest[1]) ** 2)
                
                if dist < best_dist:
                    best_dist = dist
                    best_pos = closest
        
        return best_pos

    def _closest_point_on_segment(self, px, py, x1, y1, x2, y2) -> Point:
        """Find the closest point on a line segment to a given point."""
        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy
        
        if length_sq == 0:
            return (x1, y1)
        
        t = max(0, min(1, ((px - x1) * dx + (py - y1) * dy) / length_sq))
        return (x1 + t * dx, y1 + t * dy)

    def _select_sector_at(self, pos: Point):
        for sector in reversed(self.sectors):  # last drawn gets priority
            if sector.contains(pos):
                self._select_sector(sector)
                return
        self._select_sector(None)

    def _delete_sector_at(self, pos: Point):
        for idx in range(len(self.sectors) - 1, -1, -1):
            if self.sectors[idx].contains(pos):
                removed = self.sectors.pop(idx)
                if removed == self.selected_sector:
                    self._select_sector(None)
                return

    def _select_sector(self, sector: Optional[SectorData]):
        self.selected_sector = sector
        self.selected_sectors = [sector] if sector else []
        self.selected_wall_index = None
        self.properties.set_sector(sector)
        self.wall_properties.set_wall(None)
        self.vertex_editor.set_sector(sector)
        self._refresh_scene()

    def _select_wall(self, sector: SectorData, wall_index: int):
        self.selected_sector = sector
        self.selected_wall_index = wall_index
        self.properties.set_sector(sector)
        self.vertex_editor.set_sector(sector)
        if 0 <= wall_index < len(sector.walls):
            self.wall_properties.set_wall(sector.walls[wall_index], wall_index)
        self._refresh_scene()

    def _try_select_wall(self, pos: Point) -> bool:
        """Try to select a wall near pos. Returns True if wall selected."""
        threshold = 10.0  # pixels
        for sector in reversed(self.sectors):
            for i, wall in enumerate(sector.walls):
                dist = point_segment_distance(pos, wall.start, wall.end)
                if dist < threshold:
                    self._select_wall(sector, i)
                    return True
        return False

    def _delete_selected_sector(self):
        # First, check if there are selected vertices (from marquee)
        if self.selected_vertices:
            self._delete_selected_vertices()
            return
        
        # Otherwise delete selected sector
        if not self.selected_sector:
            return
        if self.selected_sector in self.sectors:
            self.sectors.remove(self.selected_sector)
        self._select_sector(None)
        self._refresh_scene()

    def _start_grab(self, pos: Point):
        """Find nearest vertex or sector center to grab."""
        if not self.selected_sectors and not self.selected_sector:
            return
        
        # Scale mode - store original positions and start scaling
        if self.scale_mode:
            self._start_scale(pos)
            return
        
        # Check if clicking near multi-selection centroid (for multi grab)
        if len(self.selected_sectors) > 1:
            multi_center = self._get_multi_centroid()
            dist_to_multi = math.hypot(pos[0] - multi_center[0], pos[1] - multi_center[1])
            if dist_to_multi < 30:
                self.grab_mode_type = "multi"
                self.grab_start_pos = pos
                self.grab_origin = multi_center
                return
        
        # Check if clicking near sector center (for sector grab)
        if self.selected_sector:
            center = self._get_sector_centroid(self.selected_sector)
            dist_to_center = math.hypot(pos[0] - center[0], pos[1] - center[1])
            if dist_to_center < 25:
                self.grab_mode_type = "sector"
                self.grab_start_pos = pos
                self.grab_origin = center
                return
        
        # Otherwise try vertex grab
        if not self.selected_sector:
            return
        verts = self.selected_sector.vertices()
        if not verts:
            return
        # Find closest vertex
        min_dist = float("inf")
        nearest_idx = None
        for i, v in enumerate(verts):
            d = math.hypot(v[0] - pos[0], v[1] - pos[1])
            if d < min_dist:
                min_dist = d
                nearest_idx = i
        # Threshold: only grab if within 20 units
        if nearest_idx is not None and min_dist < 20:
            self.grab_mode_type = "vertex"
            self.grabbed_vertex_index = nearest_idx
            self.grab_origin = verts[nearest_idx]

    def _start_scale(self, pos: Point):
        """Initialize scaling mode with stored positions."""
        if not self.selected_sectors:
            return
        
        center = self._get_multi_centroid()
        self.grab_start_pos = pos
        self.grab_origin = center
        
        # Store original positions for all selected sectors
        self._stored_positions = {"center": center}
        for sector in self.selected_sectors:
            self._stored_positions[sector.sector_id] = [
                (wall.start, wall.end) for wall in sector.walls
            ]

    def _move_vertex(self, idx: int, new_pos: Point):
        """Move vertex at idx to new_pos, updating adjacent walls."""
        if not self.selected_sector:
            return
        walls = self.selected_sector.walls
        if idx < 0 or idx >= len(walls):
            return
        # Update this wall's start
        walls[idx].start = new_pos
        # Update previous wall's end
        prev_idx = (idx - 1) % len(walls)
        walls[prev_idx].end = new_pos

    def _toggle_sector_selection(self, pos: Point):
        """Alt+click: toggle sector in multi-selection."""
        for sector in reversed(self.sectors):
            if sector.contains(pos):
                if sector in self.selected_sectors:
                    self.selected_sectors.remove(sector)
                    if sector == self.selected_sector:
                        self.selected_sector = self.selected_sectors[0] if self.selected_sectors else None
                else:
                    self.selected_sectors.append(sector)
                    self.selected_sector = sector
                self.properties.set_sector(self.selected_sector)
                self.vertex_editor.set_sector(self.selected_sector)
                self._refresh_scene()
                return

    def _get_sector_centroid(self, sector: SectorData) -> Point:
        """Calculate centroid of a sector."""
        verts = sector.vertices()
        if not verts:
            return (0.0, 0.0)
        cx = sum(p[0] for p in verts) / len(verts)
        cy = sum(p[1] for p in verts) / len(verts)
        return (cx, cy)

    def _get_multi_centroid(self) -> Point:
        """Calculate combined centroid of all selected sectors."""
        all_verts = []
        for sector in self.selected_sectors:
            all_verts.extend(sector.vertices())
        if not all_verts:
            return (0.0, 0.0)
        cx = sum(p[0] for p in all_verts) / len(all_verts)
        cy = sum(p[1] for p in all_verts) / len(all_verts)
        return (cx, cy)

    def _move_sector(self, sector: SectorData, dx: float, dy: float):
        """Move all vertices of a sector by delta."""
        for wall in sector.walls:
            wall.start = (wall.start[0] + dx, wall.start[1] + dy)
            wall.end = (wall.end[0] + dx, wall.end[1] + dy)

    def _handle_sector_drag(self, pos: Point, shift_held: bool):
        """Handle dragging sector(s) by center."""
        if not self.grab_start_pos:
            return
        
        if shift_held and self.grab_origin:
            pos = snap_to_angle(self.grab_origin, pos)
        
        dx = pos[0] - self.grab_start_pos[0]
        dy = pos[1] - self.grab_start_pos[1]
        
        sectors_to_move = self.selected_sectors if self.grab_mode_type == "multi" else [self.selected_sector]
        for sector in sectors_to_move:
            if sector:
                self._move_sector(sector, dx, dy)
        
        self.grab_start_pos = pos

    _stored_positions: Optional[dict] = None  # For scale mode

    def _handle_scale_drag(self, pos: Point, shift_held: bool):
        """Handle scaling selected sectors."""
        if not self.grab_start_pos or not self._stored_positions:
            return
        
        # Calculate scale factor based on mouse distance from center
        center = self._stored_positions["center"]
        initial_dist = math.hypot(
            self.grab_start_pos[0] - center[0],
            self.grab_start_pos[1] - center[1]
        )
        current_dist = math.hypot(
            pos[0] - center[0],
            pos[1] - center[1]
        )
        
        if initial_dist < 1:
            initial_dist = 1
        
        scale = current_dist / initial_dist
        
        # Snap scale to nice values if shift held
        if shift_held:
            scale = round(scale * 4) / 4  # Snap to 0.25 increments
        
        # Apply scale to stored positions
        for sector in self.selected_sectors:
            if sector.sector_id not in self._stored_positions:
                continue
            original_walls = self._stored_positions[sector.sector_id]
            for i, wall in enumerate(sector.walls):
                orig_start, orig_end = original_walls[i]
                # Scale relative to center
                wall.start = (
                    center[0] + (orig_start[0] - center[0]) * scale,
                    center[1] + (orig_start[1] - center[1]) * scale
                )
                wall.end = (
                    center[0] + (orig_end[0] - center[0]) * scale,
                    center[1] + (orig_end[1] - center[1]) * scale
                )

    def _on_properties_changed(self):
        if not self.selected_sector:
            return
        self.properties.apply_to_sector()
        self._refresh_scene()

    def _on_neighbor_changed(self, wall_index: int, neighbor: Optional[int]):
        if not self.selected_sector:
            return
        if 0 <= wall_index < len(self.selected_sector.walls):
            self.selected_sector.walls[wall_index].neighbor_id = neighbor
        self._refresh_scene()

    def _on_wall_properties_changed(self):
        if not self.selected_sector or self.selected_wall_index is None:
            return
        self._refresh_scene()

    def _on_vertices_changed(self):
        """Called when vertex coordinates are edited manually."""
        if self.selected_sector:
            self.properties._populate_walls()  # Refresh wall table
        self._refresh_scene()

    def _on_vertex_selected(self, vertex_index: int):
        """Called when a vertex is selected in the vertex editor table."""
        self._refresh_scene()

    def _on_textures_changed(self, textures: List[str]):
        self.textures = textures
        self.wall_properties.set_textures(textures)

    def _try_wall_texture_shortcut(self, point: QtCore.QPointF, button: QtCore.Qt.MouseButton) -> bool:
        """Shift + right click: apply one texture to all walls in clicked sector."""
        modifiers = QtWidgets.QApplication.keyboardModifiers()
        if button != QtCore.Qt.RightButton or not (modifiers & QtCore.Qt.ShiftModifier):
            return False

        pos = (point.x(), point.y())
        sector = self._sector_at(pos)
        if not sector:
            return False

        text, ok = QtWidgets.QInputDialog.getText(
            self, "Set wall texture", "Texture name to apply to all walls:", QtWidgets.QLineEdit.Normal, ""
        )
        if not ok:
            return True
        for wall in sector.walls:
            wall.upper_texture = text
            wall.middle_texture = text
            wall.lower_texture = text
        self._select_sector(sector)
        self._refresh_scene()
        return True

    # Map IO
    def _new_map(self):
        self.sectors.clear()
        self.current_vertices.clear()
        self._select_sector(None)
        self._refresh_scene()

    def _load_map(self):
        # Use default map directory
        start_dir = str(DEFAULT_MAP_DIR) if DEFAULT_MAP_DIR.exists() else ""
        path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Load map", start_dir, filter="JSON Files (*.json)")
        if not path:
            return
        try:
            with open(path, "r", encoding="utf-8") as f:
                data = json.load(f)
            self.sectors = [SectorData.from_dict(s) for s in data.get("sectors", [])]
            self.textures = data.get("textures", self.textures)
            self.texture_manager.set_textures(self.textures)
            self.wall_properties.set_textures(self.textures)
            self._select_sector(None if not self.sectors else self.sectors[0])
        except Exception as exc:  # pylint: disable=broad-except
            QtWidgets.QMessageBox.critical(self, "Load failed", str(exc))
        self._refresh_scene()

    def _save_map(self):
        # Create default map directory if it doesn't exist
        DEFAULT_MAP_DIR.mkdir(parents=True, exist_ok=True)
        start_dir = str(DEFAULT_MAP_DIR)
        path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Save map", start_dir, filter="JSON Files (*.json)")
        if not path:
            return
        try:
            data = {
                "sectors": [s.to_dict() for s in self.sectors],
                "textures": self.textures
            }
            with open(path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
       
        except Exception as exc:  # pylint: disable=broad-except
            QtWidgets.QMessageBox.critical(self, "Save failed", str(exc))

    # Rendering
    def _draw_grid(self):
        rect = self.scene.sceneRect()
        
        # Minor grid (every 64 units)
        minor_step = 64
        minor_pen = QtGui.QPen(QtGui.QColor(50, 50, 50, 60))
        minor_pen.setWidthF(0.5)
        
        x = int(rect.left() // minor_step) * minor_step
        while x < rect.right():
            self.scene.addLine(x, rect.top(), x, rect.bottom(), minor_pen)
            x += minor_step
        y = int(rect.top() // minor_step) * minor_step
        while y < rect.bottom():
            self.scene.addLine(rect.left(), y, rect.right(), y, minor_pen)
            y += minor_step
        
        # Major grid (every 256 units) - более заметная
        major_step = 256
        major_pen = QtGui.QPen(QtGui.QColor(80, 80, 80, 120))
        major_pen.setWidthF(1.0)
        
        x = int(rect.left() // major_step) * major_step
        while x < rect.right():
            self.scene.addLine(x, rect.top(), x, rect.bottom(), major_pen)
            # Подпись координаты X
            if -2000 < x < 2000:
                label = self.scene.addText(str(int(x)))
                label.setPos(x + 2, 2)
                label.setDefaultTextColor(QtGui.QColor(100, 100, 100))
                label.setScale(0.8)
            x += major_step
        
        y = int(rect.top() // major_step) * major_step
        while y < rect.bottom():
            self.scene.addLine(rect.left(), y, rect.right(), y, major_pen)
            # Подпись координаты Y
            if -2000 < y < 2000 and y != 0:
                label = self.scene.addText(str(int(y)))
                label.setPos(2, y + 2)
                label.setDefaultTextColor(QtGui.QColor(100, 100, 100))
                label.setScale(0.8)
            y += major_step
        
        # Axes (X=0 and Y=0) - яркие оси
        axis_pen = QtGui.QPen(QtGui.QColor(150, 80, 80, 200))  # X axis - красноватая
        axis_pen.setWidthF(2.0)
        self.scene.addLine(rect.left(), 0, rect.right(), 0, axis_pen)
        
        axis_pen_y = QtGui.QPen(QtGui.QColor(80, 150, 80, 200))  # Y axis - зеленоватая
        axis_pen_y.setWidthF(2.0)
        self.scene.addLine(0, rect.top(), 0, rect.bottom(), axis_pen_y)
        
        # Origin marker (0,0)
        origin_color = QtGui.QColor(255, 255, 255, 200)
        self.scene.addEllipse(-5, -5, 10, 10, QtGui.QPen(origin_color), QtGui.QBrush(origin_color))
        origin_label = self.scene.addText("0,0")
        origin_label.setPos(8, 8)
        origin_label.setDefaultTextColor(QtGui.QColor(200, 200, 200))

    def _refresh_scene(self):
        self.scene.clear()
        self._draw_grid()
        for sector in self.sectors:
            self._draw_sector(sector)
        if self.mode_draw and len(self.current_vertices) > 0:
            self._draw_preview()
        
        # Draw circle/line preview while dragging
        if (self.mode_circle or self.mode_line or self.mode_stairs) and self.drag_start and self.drag_current:
            self._draw_shape_preview()
        
        # Draw marquee selection rectangle
        if self.mode_marquee and self.marquee_start and self.marquee_current:
            self._draw_marquee_preview()
        
        # Draw selected vertices (from marquee)
        if self.selected_vertices:
            self._draw_selected_vertices()
        
        # Draw multi-selection centroid marker
        if (self.mode_grab or self.scale_mode) and len(self.selected_sectors) > 1:
            center = self._get_multi_centroid()
            # Large white cross for multi-grab center
            center_pen = QtGui.QPen(QtGui.QColor(255, 255, 255), 3)
            self.scene.addLine(center[0] - 20, center[1], center[0] + 20, center[1], center_pen)
            self.scene.addLine(center[0], center[1] - 20, center[0], center[1] + 20, center_pen)
            # Circle around it
            self.scene.addEllipse(center[0] - 15, center[1] - 15, 30, 30,
                                   QtGui.QPen(QtGui.QColor(255, 255, 255, 180), 2))

        # Draw split points highlight
        if self.mode_split and self.split_points:
            for sector, idx in self.split_points:
                if sector and 0 <= idx < len(sector.walls):
                    vx, vy = sector.walls[idx].start
                    pen = QtGui.QPen(QtGui.QColor(50, 100, 255), 3)
                    brush = QtGui.QBrush(QtGui.QColor(50, 100, 255, 180))
                    self.scene.addEllipse(vx - 12, vy - 12, 24, 24, pen, brush)

    def _sector_at(self, pos: Point) -> Optional[SectorData]:
        for sector in reversed(self.sectors):
            if sector.contains(pos):
                return sector
        return None

    def _draw_sector(self, sector: SectorData):
        polygon = QtGui.QPolygonF([QtCore.QPointF(x, y) for x, y in sector.vertices()])
        fill = light_to_color(sector.light_level)
        brush = QtGui.QBrush(fill)
        pen = QtGui.QPen(QtGui.QColor(120, 120, 120))
        pen.setWidthF(0.0)
        poly_item = self.scene.addPolygon(polygon, pen, brush)
        
        # Highlight selected sectors
        is_primary = (sector == self.selected_sector)
        is_multi = (sector in self.selected_sectors)
        if is_primary:
            poly_item.setPen(QtGui.QPen(QtGui.QColor(255, 200, 0), 2))  # Yellow for primary
        elif is_multi:
            poly_item.setPen(QtGui.QPen(QtGui.QColor(255, 150, 50), 2))  # Orange for multi-selected
        # Draw walls over polygon
        for i, wall in enumerate(sector.walls):
            is_selected_wall = (sector == self.selected_sector and i == self.selected_wall_index)
            if is_selected_wall:
                pen_line = QtGui.QPen(QtGui.QColor(255, 255, 0), 4)  # Yellow highlight
            else:
                pen_line = QtGui.QPen(wall_color(wall.neighbor_id is not None))
                pen_line.setWidth(2)
            self.scene.addLine(wall.start[0], wall.start[1], wall.end[0], wall.end[1], pen_line)
        
        # Draw vertices - always show for selected sector, highlight selected vertex
        if sector == self.selected_sector:
            selected_vertex = self.vertex_editor.get_selected_vertex()
            for i, v in enumerate(sector.vertices()):
                radius = 6
                # Color priority: grabbed (cyan) > selected in table (bright green) > grab mode (red) > normal (small gray)
                if i == self.grabbed_vertex_index:
                    color = QtGui.QColor(0, 255, 255)  # Cyan - being dragged
                    radius = 8
                elif i == selected_vertex:
                    color = QtGui.QColor(50, 255, 50)  # Bright green - selected in vertex table
                    radius = 10
                elif self.mode_grab:
                    color = QtGui.QColor(255, 100, 100)  # Red - grab mode
                else:
                    color = QtGui.QColor(200, 200, 200)  # Gray - normal
                    radius = 4
                self.scene.addEllipse(v[0] - radius, v[1] - radius, radius * 2, radius * 2,
                                       QtGui.QPen(color), QtGui.QBrush(color))
                # Show vertex number near the point
                if i == selected_vertex or self.mode_grab:
                    label = self.scene.addText(str(i))
                    label.setPos(v[0] + radius + 2, v[1] - 8)
                    label.setDefaultTextColor(color)
        
        # Label id at centroid and show grab center point
        if sector.vertices():
            cx = sum(p[0] for p in sector.vertices()) / len(sector.vertices())
            cy = sum(p[1] for p in sector.vertices()) / len(sector.vertices())
            text_item = self.scene.addText(str(sector.sector_id))
            text_item.setPos(cx + 10, cy - 10)
            text_item.setDefaultTextColor(QtGui.QColor(255, 255, 0))
            
            # Draw center grab point for selected sectors in grab/scale mode
            if (self.mode_grab or self.scale_mode) and sector in self.selected_sectors:
                center_color = QtGui.QColor(255, 100, 255)  # Magenta for center
                radius = 8
                self.scene.addEllipse(cx - radius, cy - radius, radius * 2, radius * 2,
                                       QtGui.QPen(center_color, 2), QtGui.QBrush(QtGui.QColor(255, 100, 255, 100)))
                # Cross marker
                cross_pen = QtGui.QPen(center_color, 2)
                self.scene.addLine(cx - 12, cy, cx + 12, cy, cross_pen)
                self.scene.addLine(cx, cy - 12, cx, cy + 12, cross_pen)

    def _draw_preview(self):
        pen = QtGui.QPen(QtGui.QColor(80, 200, 255))
        pen.setWidth(2)
        verts = self.current_vertices
        
        # Draw lines between vertices
        for i in range(len(verts) - 1):
            a = verts[i]
            b = verts[i + 1]
            self.scene.addLine(a[0], a[1], b[0], b[1], pen)
        
        # Draw visible vertex points
        for i, v in enumerate(verts):
            # First vertex is special (closing point)
            if i == 0:
                color = QtGui.QColor(80, 255, 80)  # Green for first vertex
                radius = 8
            else:
                color = QtGui.QColor(80, 200, 255)  # Cyan for other vertices
                radius = 6
            self.scene.addEllipse(v[0] - radius, v[1] - radius, radius * 2, radius * 2,
                                   QtGui.QPen(color), QtGui.QBrush(color))
            # Show vertex number
            label = self.scene.addText(str(i))
            label.setPos(v[0] + radius + 2, v[1] - 8)
            label.setDefaultTextColor(color)

    def _draw_shape_preview(self):
        """Draw preview for circle or line mode while dragging."""
        if not self.drag_start or not self.drag_current:
            return
        
        pen = QtGui.QPen(QtGui.QColor(255, 180, 50))  # Orange preview
        pen.setWidth(2)
        pen.setStyle(QtCore.Qt.DashLine)
        brush = QtGui.QBrush(QtGui.QColor(255, 180, 50, 40))
        
        if self.mode_circle:
            cx, cy = self.drag_start
            ex, ey = self.drag_current
            radius = math.sqrt((ex - cx) ** 2 + (ey - cy) ** 2)
            
            # Draw circle outline
            self.scene.addEllipse(cx - radius, cy - radius, radius * 2, radius * 2, pen, brush)
            
            # Draw center point
            center_pen = QtGui.QPen(QtGui.QColor(255, 255, 0))
            center_brush = QtGui.QBrush(QtGui.QColor(255, 255, 0))
            self.scene.addEllipse(cx - 4, cy - 4, 8, 8, center_pen, center_brush)
            
            # Draw radius line
            self.scene.addLine(cx, cy, ex, ey, pen)
            
            # Show segment count
            seg_label = self.scene.addText(f"{self.circle_segments} segments")
            seg_label.setPos(cx + 10, cy + 10)
            seg_label.setDefaultTextColor(QtGui.QColor(255, 200, 100))
            
        elif self.mode_line:
            x1, y1 = self.drag_start
            x2, y2 = self.drag_current
            
            length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)
            if length < 1:
                return
            
            thickness = 32.0
            dx = x2 - x1
            dy = y2 - y1
            px = -dy / length * (thickness / 2)
            py = dx / length * (thickness / 2)
            
            # Create preview rectangle
            polygon = QtGui.QPolygonF([
                QtCore.QPointF(x1 + px, y1 + py),
                QtCore.QPointF(x1 - px, y1 - py),
                QtCore.QPointF(x2 - px, y2 - py),
                QtCore.QPointF(x2 + px, y2 + py),
            ])
            self.scene.addPolygon(polygon, pen, brush)
            
            # Draw center line
            center_pen = QtGui.QPen(QtGui.QColor(255, 255, 0))
            center_pen.setWidth(1)
            self.scene.addLine(x1, y1, x2, y2, center_pen)
            
        elif self.mode_stairs:
            x1, y1 = self.drag_start
            x2, y2 = self.drag_current
            
            length = math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)
            if length < 1:
                return
            
            thickness = 64.0
            num_steps = self.stairs_steps_spin.value()
            dx = x2 - x1
            dy = y2 - y1
            px = -dy / length * (thickness / 2)
            py = dx / length * (thickness / 2)
            
            # Draw each step preview
            for i in range(num_steps):
                t1 = i / num_steps
                t2 = (i + 1) / num_steps
                start_x = x1 + t1 * dx
                start_y = y1 + t1 * dy
                end_x = x1 + t2 * dx
                end_y = y1 + t2 * dy
                
                polygon = QtGui.QPolygonF([
                    QtCore.QPointF(start_x + px, start_y + py),
                    QtCore.QPointF(start_x - px, start_y - py),
                    QtCore.QPointF(end_x - px, end_y - py),
                    QtCore.QPointF(end_x + px, end_y + py),
                ])
                self.scene.addPolygon(polygon, pen, brush)
                
                # Draw center line for each step
                center_pen = QtGui.QPen(QtGui.QColor(255, 255, 0))
                center_pen.setWidth(1)
                self.scene.addLine(start_x, start_y, end_x, end_y, center_pen)

    def _draw_marquee_preview(self):
        """Draw the marquee selection rectangle."""
        if not self.marquee_start or not self.marquee_current:
            return
        
        x1, y1 = self.marquee_start
        x2, y2 = self.marquee_current
        
        pen = QtGui.QPen(QtGui.QColor(100, 150, 255))  # Blue dashed
        pen.setWidth(2)
        pen.setStyle(QtCore.Qt.DashLine)
        brush = QtGui.QBrush(QtGui.QColor(100, 150, 255, 30))
        
        rect = QtCore.QRectF(min(x1, x2), min(y1, y2), abs(x2 - x1), abs(y2 - y1))
        self.scene.addRect(rect, pen, brush)

    def _draw_selected_vertices(self):
        """Draw highlight on selected vertices from marquee selection."""
        for sector, v_idx in self.selected_vertices:
            if v_idx < len(sector.walls):
                vx, vy = sector.walls[v_idx].start
                # Red highlight for selected vertices
                radius = 10
                pen = QtGui.QPen(QtGui.QColor(255, 50, 50), 3)
                brush = QtGui.QBrush(QtGui.QColor(255, 50, 50, 100))
                self.scene.addEllipse(vx - radius, vy - radius, radius * 2, radius * 2, pen, brush)

    def _split_sector_by_points(self, v1: Tuple[SectorData, int], v2: Tuple[SectorData, int]):
        """Split a sector by two vertices, creating two new sectors and portals."""
        sector, idx1 = v1
        _, idx2 = v2
        if sector is None or idx1 == idx2 or len(sector.walls) < 4:
            return
        n = len(sector.walls)
        # Ensure idx1 < idx2 in circular order
        path1 = []
        i = idx1
        while True:
            path1.append(i)
            if i == idx2:
                break
            i = (i + 1) % n
        path2 = []
        i = idx2
        while True:
            path2.append(i)
            if i == idx1:
                break
            i = (i + 1) % n
        # Build vertices for each new sector
        verts1 = [sector.walls[i].start for i in path1]
        verts2 = [sector.walls[i].start for i in path2]
        if len(verts1) < 3 or len(verts2) < 3:
            return
        # Create portal walls
        portal1 = WallData(start=verts1[-1], end=verts1[0], neighbor_id=None)
        portal2 = WallData(start=verts2[-1], end=verts2[0], neighbor_id=None)
        # Build new sectors
        walls1 = [WallData(start=verts1[i], end=verts1[(i+1)%len(verts1)]) for i in range(len(verts1)-1)] + [portal1]
        walls2 = [WallData(start=verts2[i], end=verts2[(i+1)%len(verts2)]) for i in range(len(verts2)-1)] + [portal2]
        # Remove old sector, add new
        if sector in self.sectors:
            self.sectors.remove(sector)
        new_id1 = max([s.sector_id for s in self.sectors]+[0])+1
        new_id2 = new_id1+1
        s1 = SectorData(sector_id=new_id1, walls=walls1, floor_height=sector.floor_height, ceiling_height=sector.ceiling_height, light_level=sector.light_level, floor_texture=sector.floor_texture, ceiling_texture=sector.ceiling_texture, flags=sector.flags)
        s2 = SectorData(sector_id=new_id2, walls=walls2, floor_height=sector.floor_height, ceiling_height=sector.ceiling_height, light_level=sector.light_level, floor_texture=sector.floor_texture, ceiling_texture=sector.ceiling_texture, flags=sector.flags)
        # Set portals
        s1.walls[-1].neighbor_id = new_id2
        s2.walls[-1].neighbor_id = new_id1
        self.sectors.append(s1)
        self.sectors.append(s2)
        self._select_sector(None)
        self._refresh_scene()

def main():
    app = QtWidgets.QApplication([])
    window = MainWindow()
    window.resize(1200, 800)
    window.show()
    app.exec()


if __name__ == "__main__":
    main()
