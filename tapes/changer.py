from tapes.internal import changer
from dataclasses import dataclass
from typing import List
from enum import Enum

class LibraryElementType(Enum):
    DRIVE, SLOT, IE_STATION, ROBOT = range(4)

@dataclass
class LibraryElement:
    element_type: LibraryElementType
    address: int
    tape_source_address: int
    is_full: bool
    barcode: str

@dataclass
class LibraryInventory:
    robots: List[LibraryElement]
    slots: List[LibraryElement]
    drives: List[LibraryElement]
    ie_stations: List[LibraryElement]
    @property
    def all_elements(self):
        return self.robots + self.slots + self.drives + self.ie_stations
    @property
    def address_map(self):
        return {elem.address: elem for elem in self.all_elements}
    @property
    def barcode_map(self):
        return {elem.barcode: elem for elem in self.all_elements if elem.barcode is not None}

class Changer:
    def __init__(self, device_path):
        self.dev = device_path
    def get_inventory(self):
        raw = changer._get_inventory(self.dev)
        return LibraryInventory(
            robots=[LibraryElement(element_type=LibraryElementType.ROBOT, **x) for x in raw['robots']],
            slots=[LibraryElement(element_type=LibraryElementType.SLOT, **x) for x in raw['slots']],
            drives=[LibraryElement(element_type=LibraryElementType.DRIVE, **x) for x in raw['drives']],
            ie_stations=[LibraryElement(element_type=LibraryElementType.IE_STATION, **x) for x in raw['ie_stations']],
        )
    def move_cartridge(self, source_address, target_address, robot_address):
        changer._move_cartridge(self.dev, source_address, target_address, robot_address)
    def load_cartridge(self, barcode, drive_address, robot_address):
        inv = self.get_inventory()
        robot = inv.address_map[robot_address]
        drive = inv.address_map[drive_address]
        slot = inv.barcode_map.get(barcode)
        if slot is None:
            raise Exception('Given tape is not available')
        if slot.element_type == LibraryElementType.ROBOT and slot != robot:
            raise Exception('Given tape is in another robot')
        if slot == drive:
            return
        if drive.is_full:
            self.unload_cartridge(drive_address, robot_address)
        self.move_cartridge(slot.address, drive.address, robot.address)
    def unload_cartridge(self, drive_address, robot_address):
        inv = self.get_inventory()
        drive = inv.address_map[drive_address]
        if not drive.is_full:
            return
        if drive.tape_source_address is not None and drive.tape_source_address > 0:
            source_slot = inv.address_map[drive.tape_source_address]
            if not source_slot.is_full:
                return self.move_cartridge(drive.address, source_slot.address, robot_address)
        empty_slots = [x for x in inv.slots if not x.is_full]
        if len(empty_slots) == 0:
            raise Exception('No slot is empty')
        return self.move_cartridge(drive.address, empty_slots[0].address, robot_address)
