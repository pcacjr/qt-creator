#include "virtualserialdevice.h"
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

namespace SymbianUtils {

bool VirtualSerialDevice::isSequential() const
{
    return true;
}

VirtualSerialDevice::VirtualSerialDevice(const QString &aPortName, QObject *parent) :
    QIODevice(parent), portName(aPortName), lock(QMutex::NonRecursive), emittingBytesWritten(false), waiterForBytesWritten(NULL)
{
    platInit();
}

const QString& VirtualSerialDevice::getPortName() const
{
    return portName;
}

void VirtualSerialDevice::close()
{
    if (isOpen()) {
        Q_ASSERT(thread() == QThread::currentThread()); // My brain will explode otherwise
        delete waiterForBytesWritten;
        waiterForBytesWritten = NULL;
        QMutexLocker locker(&lock);
        QIODevice::close();
        platClose();
    }
}

void VirtualSerialDevice::emitBytesWrittenIfNeeded(QMutexLocker& locker, qint64 len)
{
    if (waiterForBytesWritten) {
        waiterForBytesWritten->wakeAll();
    }
    if (!emittingBytesWritten) {
        emittingBytesWritten = true;
        locker.unlock();
        emit bytesWritten(len);
        locker.relock();
        emittingBytesWritten = false;
    }
}

} // namespace SymbianUtils