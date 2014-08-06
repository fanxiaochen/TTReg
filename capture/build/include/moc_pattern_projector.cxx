/****************************************************************************
** Meta object code from reading C++ file 'pattern_projector.h'
**
** Created: Wed Aug 6 13:41:55 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../KingFisher/capture/include/pattern_projector.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pattern_projector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PatternProjector[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      23,   18,   17,   17, 0x05,
      59,   47,   17,   17, 0x05,
      92,   85,   17,   17, 0x05,
     129,  124,   17,   17, 0x05,
     150,   17,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
     176,  166,   17,   17, 0x0a,
     204,   17,   17,   17, 0x0a,
     225,   85,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_PatternProjector[] = {
    "PatternProjector\0\0view\0timeToGrabSnapshot(int)\0"
    "view,stripe\0timeToGrabStripe(int,int)\0"
    "stripe\0timeToShowImageMessage(QString)\0"
    "time\0timeToRotate(double)\0imagesGrabbed()\0"
    "view,time\0projectSnapshot(int,double)\0"
    "projectFirstStripe()\0projectNextStripe(int)\0"
};

void PatternProjector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PatternProjector *_t = static_cast<PatternProjector *>(_o);
        switch (_id) {
        case 0: _t->timeToGrabSnapshot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->timeToGrabStripe((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->timeToShowImageMessage((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->timeToRotate((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->imagesGrabbed(); break;
        case 5: _t->projectSnapshot((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 6: _t->projectFirstStripe(); break;
        case 7: _t->projectNextStripe((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PatternProjector::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PatternProjector::staticMetaObject = {
    { &QGLWidget::staticMetaObject, qt_meta_stringdata_PatternProjector,
      qt_meta_data_PatternProjector, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PatternProjector::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PatternProjector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PatternProjector::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PatternProjector))
        return static_cast<void*>(const_cast< PatternProjector*>(this));
    return QGLWidget::qt_metacast(_clname);
}

int PatternProjector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void PatternProjector::timeToGrabSnapshot(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PatternProjector::timeToGrabStripe(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PatternProjector::timeToShowImageMessage(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PatternProjector::timeToRotate(double _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void PatternProjector::imagesGrabbed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
