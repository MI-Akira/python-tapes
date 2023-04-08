from tapes.internal import tape
from dataclasses import dataclass
from typing import List
from enum import Enum
import math

class TapePartitionType(Enum):
    UNKNOWN, IDP, SDP, FDP = range(4)

class TapePartitionMethod(Enum):
    UNKNOWN, WRAP_WISE, LONGITUDE = range(3)

class TapeLogicalPosition(Enum):
    BLOCK, FILE = range(2)

@dataclass
class TapePartitionLayout:
    max_partitions: int
    active_partition: int
    partition_method: TapePartitionMethod
    partitions: List[int]

@dataclass
class TapeTypeProperties:
    wraps: int
    size: int
    name: str
    @property
    def wrap_size(self):
        return self.size / self.wraps

class Tape:
    def __init__(self, device_path):
        self.dev = device_path
    def sync(self):
        tape._sync_tape(self.dev)
    def get_position_block(self):
        return tape._get_tape_position(self.dev)["curpos"]
    def set_position_block(self, block_id):
        tape._set_tape_position(self.dev, TapeLogicalPosition.BLOCK.value, block_id)
    def set_position_file(self, file_id):
        tape._set_tape_position(self.dev, TapeLogicalPosition.FILE.value, file_id)
    def set_position_to_eod(self):
        tape._send_tape_operation(self.dev, 32, 0)
    def set_partition(self, part_id):
        tape._set_active_partition(self.dev, part_id)
    def get_partition(self):
        return tape._query_partitions(self.dev)['active_partition']
    def get_partition_layout(self):
        media = self.get_tape_type_properties()
        raw = tape._query_partitions(self.dev)
        raw_sizes = raw['size'][:raw['number_of_partitions']]
        scaled_sizes = [round((x * 10**raw['size_unit'])/media.wrap_size) for x in raw_sizes]
        wraps_as_gap = 2 * (len(raw_sizes) - 1)
        used_wraps = media.wraps - wraps_as_gap
        assert sum(scaled_sizes) == used_wraps
        return TapePartitionLayout(
            max_partitions=raw['max_partitions'],
            active_partition=raw['active_partition'],
            partition_method=TapePartitionMethod(raw['partition_method']),
            partitions=scaled_sizes
        )
    def create_one_partition_layout(self):
        # Most arguments are ignored if number of partitions is equal to one
        tape._partition_tape(
            self.dev,
            TapePartitionType.UNKNOWN.value, # ignored
            1, # number of partitions
            0, # size_unit, ignored
            TapePartitionMethod.WRAP_WISE.value,
            [0] # partition sizes, ignored
        )
    def create_wrap_wise_fdp_partition_layout(self):
        tape._partition_tape(
            self.dev,
            TapePartitionType.FDP.value,
            2, # number of partitions, ignored, but has to be > 1
            0, # size_unit, ignored for FDP
            TapePartitionMethod.WRAP_WISE.value,
            [0] # partition sizes, ignored for FDP
        )
    def create_wrap_wise_sdp_partition_layout(self, number_of_partitions: int):
        # size_unit and size arguments are ignored
        tape._partition_tape(
            self.dev,
            TapePartitionType.SDP.value,
            number_of_partitions,
            0, # size_unit, ifnored for SDP
            TapePartitionMethod.WRAP_WISE.value,
            [0]*number_of_partitions # partition sizes, ignored for SDP
        )
    def create_wrap_wise_idp_partition_layout(self, wraps: List[int]):
        media = self.get_tape_type_properties()
        wraps_for_gaps = 2 * (len(wraps) - 1)
        assert sum(wraps) + wraps_for_gaps == media.wraps
        part_sizes = [wraps_count * media.wrap_size for wraps_count in wraps]
        min_part = min(part_sizes)
        max_part = max(part_sizes)
        for size_unit in range(0, 12):
            if min_part / (10**size_unit) >= 1 and max_part / (10**size_unit) < 2**16 - 1:
                scaled_sizes = [math.floor(x / (10**size_unit)) for x in part_sizes]
                tape._partition_tape(
                    self.dev,
                    TapePartitionType.IDP.value,
                    len(scaled_sizes), # number of partitions
                    size_unit,
                    TapePartitionMethod.WRAP_WISE.value,
                    scaled_sizes
                )
                return
        raise Exception('Failed to find the right size_unit')
    def get_tape_type_properties(self):
        known_media = {
            (0x58, 0x58): TapeTypeProperties(name='LTO-5', wraps=80, size=1.5*10**12),
            (0x58, 0x5c): TapeTypeProperties(name='LTO-5 WORM', wraps=80, size=1.5*10**12),
            (0x5a, 0x68): TapeTypeProperties(name='LTO-6', wraps=136, size=2.5*10**12),
            (0x5a, 0x6c): TapeTypeProperties(name='LTO-6 WORM', wraps=136, size=2.5*10**12),
            (0x5c, 0x78): TapeTypeProperties(name='LTO-7', wraps=112, size=6*10**12),
            (0x5c, 0x7c): TapeTypeProperties(name='LTO-7 WORM', wraps=112, size=6*10**12),
            (0x5d, 0x78): TapeTypeProperties(name='LTO-M8', wraps=168, size=9*10**12),
            (0x5e, 0x88): TapeTypeProperties(name='LTO-8', wraps=208, size=12*10**12),
            (0x5e, 0x8c): TapeTypeProperties(name='LTO-8 WORM', wraps=208, size=12*10**12),
            (0x60, 0x98): TapeTypeProperties(name='LTO-9', wraps=280, size=18*10**12),
            (0x60, 0x9c): TapeTypeProperties(name='LTO-9 WORM', wraps=280, size=18*10**12)
        }
        params = tape._query_params(self.dev)
        return known_media.get((params['density_code'], params['medium_type']))
    def rewind(self):
        tape._send_tape_operation(self.dev, 6, 0)
    def erase(self):
        tape._send_tape_operation(self.dev, 7, 0)
    def retension(self):
        tape._send_tape_operation(self.dev, 8, 0)
    def write_end_of_file_record(self):
        tape._send_tape_operation(self.dev, 10, 0)
    def load(self):
        tape._send_tape_operation(self.dev, 15, 0)
    def unload(self):
        tape._send_tape_operation(self.dev, 16, 0)

