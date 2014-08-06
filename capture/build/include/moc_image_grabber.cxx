/****************************************************************************
** Meta object code from reading C++ file 'image_grabber.h'
**
** Created: Wed Aug 6 13:41:56 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../KingFisher/capture/include/image_grabber.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'image_grabber.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ImageGrabber[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x05,
      39,   32,   13,   13, 0x05,
      70,   58,   13,   13, 0x05,

 // slots: signature, parameters, type, tag, flags
      95,   90,   13,   13, 0x0a,
     113,   58,   13,   13, 0x0a,
     160,  133,   13,   13, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ImageGrabber[] = {
    "ImageGrabber\0\0snapshotGrabbed()\0stripe\0"
    "stripeGrabbed(int)\0view,stripe\0"
    "timeToView(int,int)\0view\0grabSnapshot(int)\0"
    "grabStripe(int,int)\0image_folder,points_folder\0"
    "save(QString,QString)\0"
};

void ImageGrabber::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ImageGrabber *_t = static_cast<ImageGrabber *>(_o);
        switch (_id) {
        case 0: _t->snapshotGrabbed(); break;
        case 1: _t->stripeGrabbed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->timeToView((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->grabSnapshot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->grabStripe((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->save((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ImageGrabber::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ImageGrabber::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ImageGrabber,
      qt_meta_data_ImageGrabber, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ImageGrabber::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ImageGrabber::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ImageGrabber::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ImageGrabber))
        return static_cast<void*>(const_cast< ImageGrabber*>(this));
    return QObject::qt_metacast(_clname);
}

int ImageGrabber::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ImageGrabber::snapshotGrabbed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void ImageGrabber::stripeGrabbed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ImageGrabber::timeToView(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_END_MOC_NAMESPACE
