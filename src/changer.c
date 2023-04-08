#include <Python.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/version.h>
#include "IBM_tape.h"
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


enum element_type {TYPE_ROBOT, TYPE_IE_STATION, TYPE_DRIVE, TYPE_SLOT};

void iterate_inventory(struct element_info* element_info, struct inventory* inventory, void (*callback)(struct element_status*, enum element_type type)) {
    for (int i = 0; i<element_info->robots; i++) (*callback)(&inventory->robot_status[i], TYPE_ROBOT);
    for (int i = 0; i<element_info->ie_stations; i++) (*callback)(&inventory->ie_status[i], TYPE_IE_STATION);
    for (int i = 0; i<element_info->drives; i++) (*callback)(&inventory->drive_status[i], TYPE_DRIVE);
    for (int i = 0; i<element_info->slots; i++) (*callback)(&inventory->slot_status[i], TYPE_SLOT);
}

void trim_barcode(struct element_status* status, enum element_type type) {
    int i = 0;
    while (status->volume[i] != ' ' && status->volume[i] != '\0') i++;
    status->volume[i] = '\0';
}

static PyObject *method_move_cartridge(PyObject *self, PyObject *args) {
    char *path;
    short int src, dest, robot;
    if(!PyArg_ParseTuple(args, "sHHH", &path, &src, &dest, &robot)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open changer ioctl device.");
        return NULL;
    }

    struct move_medium move_medium;
    move_medium.source = src;
    move_medium.destination = dest;
    move_medium.robot = robot;
    move_medium.invert = 0;

    if (ioctl(fd, SMCIOC_MOVE_MEDIUM, &move_medium)) {
        PyErr_SetString(PyExc_ValueError, "Failed to move cartridge");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *method_get_inventory(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open changer ioctl device.");
        return NULL;
    }

    struct element_info element_info;
    if (ioctl(fd, SMCIOC_ELEMENT_INFO, &element_info)) {
        PyErr_SetString(PyExc_ValueError, "Failed to list available elements in the inventory.");
        return NULL;
    }
    
    struct element_status robot_status[element_info.robots];
    struct element_status slot_status[element_info.slots];
    struct element_status ie_status[element_info.ie_stations];
    struct element_status drive_status[element_info.drives];
    struct inventory inventory;

    inventory.robot_status = robot_status;
    inventory.slot_status = slot_status;
    if (element_info.ie_stations > 0) {
        inventory.ie_status = ie_status;
    }
    inventory.drive_status = drive_status;
    
    if (ioctl(fd, SMCIOC_INVENTORY, &inventory)) {
        PyErr_SetString(PyExc_ValueError, "Failed to execute inventory command.");
        return NULL;
    }
    iterate_inventory(&element_info, &inventory, &trim_barcode);

    int err = 0;

    PyObject *robots_list = PyList_New(element_info.robots);
    for (int i = 0; i < element_info.robots; i++) {
        PyObject *robot = PyDict_New();
        err += PyDict_SetItem(robot, PyUnicode_FromString("address"), PyLong_FromLong(robot_status[i].address));
        err += PyDict_SetItem(robot, PyUnicode_FromString("tape_source_address"), PyLong_FromLong(robot_status[i].source));
        err += PyDict_SetItem(robot, PyUnicode_FromString("is_full"), PyBool_FromLong(robot_status[i].full));
        if (robot_status[i].volume[0] == '\0') err += PyDict_SetItem(robot, PyUnicode_FromString("barcode"), Py_None);
        else err += PyDict_SetItem(robot, PyUnicode_FromString("barcode"), PyUnicode_FromString((const char *) robot_status[i].volume));
        PyList_SET_ITEM(robots_list, i, robot);
    }

    PyObject *slots_list = PyList_New(element_info.slots);
    for (int i = 0; i < element_info.slots; i++) {
        PyObject *slot = PyDict_New();
        err += PyDict_SetItem(slot, PyUnicode_FromString("address"), PyLong_FromLong(slot_status[i].address));
        err += PyDict_SetItem(slot, PyUnicode_FromString("tape_source_address"), PyLong_FromLong(slot_status[i].source));
        err += PyDict_SetItem(slot, PyUnicode_FromString("is_full"), PyBool_FromLong(slot_status[i].full));
        if (slot_status[i].volume[0] == '\0') err += PyDict_SetItem(slot, PyUnicode_FromString("barcode"), Py_None);
        else err += PyDict_SetItem(slot, PyUnicode_FromString("barcode"), PyUnicode_FromString((const char *) slot_status[i].volume));
        PyList_SET_ITEM(slots_list, i, slot);
    }

    PyObject *ie_stations_list = PyList_New(element_info.ie_stations);
    for (int i = 0; i < element_info.ie_stations; i++) {
        PyObject *ie_station = PyDict_New();
        err += PyDict_SetItem(ie_station, PyUnicode_FromString("address"), PyLong_FromLong(ie_status[i].address));
        err += PyDict_SetItem(ie_station, PyUnicode_FromString("tape_source_address"), PyLong_FromLong(ie_status[i].source));
        err += PyDict_SetItem(ie_station, PyUnicode_FromString("is_full"), PyBool_FromLong(ie_status[i].full));
        if (ie_status[i].volume[0] == '\0') err += PyDict_SetItem(ie_station, PyUnicode_FromString("barcode"), Py_None);
        else err += PyDict_SetItem(ie_station, PyUnicode_FromString("barcode"), PyUnicode_FromString((const char *) ie_status[i].volume));
        PyList_SET_ITEM(ie_stations_list, i, ie_station);
    }

    PyObject *drives_list = PyList_New(element_info.drives);
    for (int i = 0; i < element_info.drives; i++) {
        PyObject *drive = PyDict_New();
        err += PyDict_SetItem(drive, PyUnicode_FromString("address"), PyLong_FromLong(drive_status[i].address));
        err += PyDict_SetItem(drive, PyUnicode_FromString("tape_source_address"), PyLong_FromLong(drive_status[i].source));
        err += PyDict_SetItem(drive, PyUnicode_FromString("is_full"), PyBool_FromLong(drive_status[i].full));
        if (drive_status[i].volume[0] == '\0') err += PyDict_SetItem(drive, PyUnicode_FromString("barcode"), Py_None);
        else err += PyDict_SetItem(drive, PyUnicode_FromString("barcode"), PyUnicode_FromString((const char *) drive_status[i].volume));
        PyList_SET_ITEM(drives_list, i, drive);
    }

    PyObject *output = PyDict_New();
    err += PyDict_SetItem(output, PyUnicode_FromString("robots"), robots_list);
    err += PyDict_SetItem(output, PyUnicode_FromString("slots"), slots_list);
    err += PyDict_SetItem(output, PyUnicode_FromString("ie_stations"), ie_stations_list);
    err += PyDict_SetItem(output, PyUnicode_FromString("drives"), drives_list);

    if (err > 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to execute inventory command.");
        return NULL;
    }

    return output;
}

static PyMethodDef changer_methods[] = {
    {"_get_inventory", method_get_inventory, METH_VARARGS, "Loads cached inventory of the library"},
    {"_move_cartridge", method_move_cartridge, METH_VARARGS, "Move cartidge from the source to the destination"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef changer_module = {
    PyModuleDef_HEAD_INIT,
    "changer",
    "Python interface for the tape changer",
    -1,
    changer_methods
};

PyMODINIT_FUNC PyInit_changer(void) {
    return PyModule_Create(&changer_module);
}
