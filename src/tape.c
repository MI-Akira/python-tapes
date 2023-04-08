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


static PyObject *method_query_partitions(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct query_partition query;

    if (ioctl(fd, STIOC_QUERY_PARTITION, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to make a query");
        return NULL;
    }

    int err = 0;
    PyObject *output = PyDict_New();
    err += PyDict_SetItem(output, PyUnicode_FromString("max_partitions"), PyLong_FromLong(query.max_partitions));
    err += PyDict_SetItem(output, PyUnicode_FromString("active_partition"), PyLong_FromLong(query.active_partition));
    err += PyDict_SetItem(output, PyUnicode_FromString("number_of_partitions"), PyLong_FromLong(query.number_of_partitions));
    err += PyDict_SetItem(output, PyUnicode_FromString("size_unit"), PyLong_FromLong(query.size_unit));

    PyObject *size_list = PyList_New(MAX_PARTITIONS);
    for (int i=0; i < MAX_PARTITIONS;i++) {
        PyList_SET_ITEM(size_list, i, PyLong_FromLong(query.size[i]));
    }
    err += PyDict_SetItem(output, PyUnicode_FromString("size"), size_list);
    err += PyDict_SetItem(output, PyUnicode_FromString("partition_method"), PyLong_FromLong(query.partition_method));

    if (err > 0) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to return results.");
        return NULL;
    }
    close(fd);

    return output;
}

static PyObject *method_set_active_partition(PyObject *self, PyObject *args) {
    char *path;
    uint8_t part;
    if(!PyArg_ParseTuple(args, "sb", &path, &part)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct set_active_partition query;
    query.partition_number = part;
    query.logical_block_id = 0L;

    if (ioctl(fd, STIOC_SET_ACTIVE_PARTITION, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to set active partition");
        return NULL;
    }
    close(fd);

    Py_RETURN_NONE;
}

static PyObject *method_set_tape_position(PyObject *self, PyObject *args) {
    char *path;
    uint8_t id_type;
    unsigned long id;
    if(!PyArg_ParseTuple(args, "sbk", &path, &id_type, &id)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct set_tape_position query;
    query.logical_id_type = id_type;
    query.logical_id = id;

    if (ioctl(fd, STIOC_LOCATE_16, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to set tape position");
        return NULL;
    }
    close(fd);

    Py_RETURN_NONE;
}

static PyObject *method_sync_tape(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    if (ioctl(fd, STIOCSYNC)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to sync tape");
        return NULL;
    }
    close(fd);

    Py_RETURN_NONE;
}

static PyObject *method_send_tape_operation(PyObject *self, PyObject *args) {
    char *path;
    short op;
    long count;

    if(!PyArg_ParseTuple(args, "shl", &path, &op, &count)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct stop query;
    query.st_op = op;
    query.st_count = count;

    if (ioctl(fd, STIOCTOP, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to send operation");
        return NULL;
    }
    close(fd);

    Py_RETURN_NONE;
}

static PyObject *method_query_params(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct stchgp_s query;

    if (ioctl(fd, STIOCQRYP, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to make a query");
        return NULL;
    }

    int err = 0;
    PyObject *output = PyDict_New();
    err += PyDict_SetItem(output, PyUnicode_FromString("autoload"), PyBool_FromLong(query.autoload));
    err += PyDict_SetItem(output, PyUnicode_FromString("buffered_mode"), PyBool_FromLong(query.buffered_mode));
    err += PyDict_SetItem(output, PyUnicode_FromString("compression"), PyBool_FromLong(query.compression));
    err += PyDict_SetItem(output, PyUnicode_FromString("trailer_labels"), PyBool_FromLong(query.trailer_labels));
    err += PyDict_SetItem(output, PyUnicode_FromString("rewind_immediate"), PyBool_FromLong(query.rewind_immediate));
    err += PyDict_SetItem(output, PyUnicode_FromString("bus_domination"), PyBool_FromLong(query.bus_domination));
    err += PyDict_SetItem(output, PyUnicode_FromString("logging"), PyBool_FromLong(query.logging));
    err += PyDict_SetItem(output, PyUnicode_FromString("write_protect"), PyBool_FromLong(query.write_protect));
    err += PyDict_SetItem(output, PyUnicode_FromString("emulate_autoloader"), PyBool_FromLong(query.emulate_autoloader));
    err += PyDict_SetItem(output, PyUnicode_FromString("wfm_immediate"), PyBool_FromLong(query.wfm_immediate));
    err += PyDict_SetItem(output, PyUnicode_FromString("limit_read_recov"), PyBool_FromLong(query.limit_read_recov));
    err += PyDict_SetItem(output, PyUnicode_FromString("limit_write_recov"), PyBool_FromLong(query.limit_write_recov));
    err += PyDict_SetItem(output, PyUnicode_FromString("data_safe_mode"), PyBool_FromLong(query.data_safe_mode));
    err += PyDict_SetItem(output, PyUnicode_FromString("disable_sim_logging"), PyBool_FromLong(query.disable_sim_logging));
    err += PyDict_SetItem(output, PyUnicode_FromString("read_sili_bit"), PyBool_FromLong(query.read_sili_bit));
    err += PyDict_SetItem(output, PyUnicode_FromString("disable_auto_drive_dump"), PyBool_FromLong(query.disable_auto_drive_dump));
    err += PyDict_SetItem(output, PyUnicode_FromString("trace"), PyBool_FromLong(query.trace));

    err += PyDict_SetItem(output, PyUnicode_FromString("acf_mode"), PyLong_FromUnsignedLong(query.acf_mode));
    err += PyDict_SetItem(output, PyUnicode_FromString("record_space_mode"), PyLong_FromUnsignedLong(query.record_space_mode));
    err += PyDict_SetItem(output, PyUnicode_FromString("logical_write_protect"), PyLong_FromUnsignedLong(query.logical_write_protect));
    err += PyDict_SetItem(output, PyUnicode_FromString("capacity_scaling"), PyLong_FromUnsignedLong(query.capacity_scaling));
    err += PyDict_SetItem(output, PyUnicode_FromString("retain_reservation"), PyLong_FromUnsignedLong(query.retain_reservation));
    err += PyDict_SetItem(output, PyUnicode_FromString("alt_pathing"), PyLong_FromUnsignedLong(query.alt_pathing));
    err += PyDict_SetItem(output, PyUnicode_FromString("medium_type"), PyLong_FromUnsignedLong(query.medium_type));
    err += PyDict_SetItem(output, PyUnicode_FromString("density_code"), PyLong_FromUnsignedLong(query.density_code));
    err += PyDict_SetItem(output, PyUnicode_FromString("read_past_filemark"), PyLong_FromUnsignedLong(query.read_past_filemark));
    err += PyDict_SetItem(output, PyUnicode_FromString("capacity_scaling_value"), PyLong_FromUnsignedLong(query.capacity_scaling_value));
    err += PyDict_SetItem(output, PyUnicode_FromString("busy_retry"), PyLong_FromUnsignedLong(query.busy_retry));
    err += PyDict_SetItem(output, PyUnicode_FromString("reserve_type"), PyLong_FromUnsignedLong(query.reserve_type));

    err += PyDict_SetItem(output, PyUnicode_FromString("hkwrd"), PyLong_FromUnsignedLong(query.hkwrd));
    err += PyDict_SetItem(output, PyUnicode_FromString("min_blksize"), PyLong_FromUnsignedLong(query.min_blksize));
    err += PyDict_SetItem(output, PyUnicode_FromString("max_blksize"), PyLong_FromUnsignedLong(query.max_blksize));
    err += PyDict_SetItem(output, PyUnicode_FromString("max_scsi_xfer"), PyLong_FromUnsignedLong(query.max_scsi_xfer));
    err += PyDict_SetItem(output, PyUnicode_FromString("blksize"), PyLong_FromLong(query.blksize));
    err += PyDict_SetItem(output, PyUnicode_FromString("volid"), PyUnicode_FromStringAndSize((const char*) &query.volid, 16));

    if (err > 0) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to return results.");
        return NULL;
    }
    close(fd);

    return output;
}

static PyObject *method_get_tape_position(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct stpos_s query;

    if (ioctl(fd, STIOCQRYPOS, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to make a query");
        return NULL;
    }

    int err = 0;
    PyObject *output = PyDict_New();
    err += PyDict_SetItem(output, PyUnicode_FromString("eot"), PyBool_FromLong(query.eot));
    err += PyDict_SetItem(output, PyUnicode_FromString("bot"), PyBool_FromLong(query.bot));

    err += PyDict_SetItem(output, PyUnicode_FromString("tapepos"), PyLong_FromUnsignedLong(query.tapepos));
    err += PyDict_SetItem(output, PyUnicode_FromString("curpos"), PyLong_FromUnsignedLong(query.curpos));
    err += PyDict_SetItem(output, PyUnicode_FromString("lbot"), PyLong_FromUnsignedLong(query.lbot));
    err += PyDict_SetItem(output, PyUnicode_FromString("num_blocks"), PyLong_FromUnsignedLong(query.num_blocks));

    err += PyDict_SetItem(output, PyUnicode_FromString("block_type"), PyLong_FromUnsignedLong(query.block_type));
    err += PyDict_SetItem(output, PyUnicode_FromString("partition_number"), PyLong_FromUnsignedLong(query.partition_number));

    if (err > 0) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to return results.");
        return NULL;
    }
    close(fd);

    return output;
}

static PyObject *method_partition_tape(PyObject *self, PyObject *args) {
    char *path;
    uint8_t partition_type;
    uint8_t partitions_count;
    uint8_t size_unit;
    uint8_t partition_method;

    PyObject *size_list;
    if(!PyArg_ParseTuple(args, "sbbbbO!", &path, &partition_type, &partitions_count, &size_unit, &partition_method, &PyList_Type, &size_list)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct tape_partition query;
    query.type = partition_type;
    query.partition_method = partition_method;
    query.number_of_partitions = partitions_count;
    query.size_unit = size_unit;

    int size_list_len = PyList_Size(size_list);
    for (int i = 0;i < MAX_PARTITIONS;i++) {
        if (i < size_list_len) {
            unsigned long len = PyLong_AsUnsignedLong(PyList_GetItem(size_list, i));
            if (len > USHRT_MAX) len = 0L;
            query.size[i] = (ushort) len;
        } else query.size[i] = 0;
    }

    if (ioctl(fd, STIOC_CREATE_PARTITION, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to create partitions");
        return NULL;
    }
    close(fd);

    Py_RETURN_NONE;
}

static PyObject *method_get_tape_ids(PyObject *self, PyObject *args) {
    char *path;
    if(!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        PyErr_SetString(PyExc_ValueError, "Failed to open ioctl device.");
        return NULL;
    }

    struct inquiry_data query;

    if (ioctl(fd, SIOC_INQUIRY, &query)) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to make a query");
        return NULL;
    }

    int err = 0;
    PyObject *output = PyDict_New();
    err += PyDict_SetItem(output, PyUnicode_FromString("vendor_id"), PyUnicode_FromStringAndSize((const char*) query.vid, VEND_ID_LEN));
    err += PyDict_SetItem(output, PyUnicode_FromString("product_id"), PyUnicode_FromStringAndSize((const char*) query.pid, PROD_ID_LEN));
    err += PyDict_SetItem(output, PyUnicode_FromString("revision"), PyUnicode_FromStringAndSize((const char*) query.revision, REV_LEN));

    if (err > 0) {
        close(fd);
        PyErr_SetString(PyExc_ValueError, "Failed to return results.");
        return NULL;
    }
    close(fd);

    return output;
}

static PyMethodDef tape_methods[] = {
    {"_query_partitions", method_query_partitions, METH_VARARGS, "Query available partitions"},
    {"_set_active_partition", method_set_active_partition, METH_VARARGS, "Set active partition"},
    {"_set_tape_position", method_set_tape_position, METH_VARARGS, "Set tape position"},
    {"_sync_tape", method_sync_tape, METH_VARARGS, "Sync buffers to the tape"},
    {"_query_params", method_query_params, METH_VARARGS, "Query params"},
    {"_get_tape_position", method_get_tape_position, METH_VARARGS, "Get tape position"},
    {"_send_tape_operation", method_send_tape_operation, METH_VARARGS, "Send a tape operation"},
    {"_partition_tape", method_partition_tape, METH_VARARGS, "Partition a tape"},
    {"_get_tape_ids", method_get_tape_ids, METH_VARARGS, "Get product and vendor id of a tape"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef tape_module = {
    PyModuleDef_HEAD_INIT,
    "tape",
    "Python interface for the tapes",
    -1,
    tape_methods
};

PyMODINIT_FUNC PyInit_tape(void) {
    return PyModule_Create(&tape_module);
}
