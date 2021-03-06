/*
 * GuLinux Planetary Imager - https://github.com/GuLinux/PlanetaryImager
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "remotedriver.h"
#include "network/protocol/driverprotocol.h"
#include "network/networkdispatcher.h"
#include <QCoreApplication>
#include "remoteimager.h"
#include "network/networkpacket.h"

using namespace std;

DPTR_IMPL(RemoteDriver) {
  QList<CameraPtr> cameras;
  DriverProtocol::DriverStatus status;
};

class RemoteCamera : public Camera {
public:
  RemoteCamera(const QString &name, qlonglong address, const NetworkDispatcherPtr &dispatcher) : _name{name}, _address{address}, _dispatcher{dispatcher} {}
  Imager * imager(const ImageHandlerPtr & imageHandler) const override;
  QString name() const override { return _name; }
private:
  const QString _name;
  const qlonglong _address;
  const NetworkDispatcherPtr _dispatcher;
};

Imager * RemoteCamera::imager(const ImageHandlerPtr& imageHandler) const
{
  return new RemoteImager{imageHandler, _dispatcher, _address};
}


RemoteDriver::RemoteDriver(const NetworkDispatcherPtr &dispatcher) : NetworkReceiver{dispatcher}, dptr()
{
  d->status.imager_running = false;
  register_handler(DriverProtocol::CameraListReply, [this](const NetworkPacketPtr &packet){
    qDebug() << "Processing cameras: " << packet;
    d->cameras.clear();
    DriverProtocol::decode(d->cameras, packet, [&](const QString &name, qlonglong address) { return make_shared<RemoteCamera>(name, address, this->dispatcher() ); });
  });
  register_handler(NetworkProtocol::HelloReply, [this](const NetworkPacketPtr &p) {
    qDebug() << "hello reply handler";
    d->status = DriverProtocol::decodeStatus(p);
  });
}

RemoteDriver::~RemoteDriver()
{
}


QList<CameraPtr> RemoteDriver::cameras() const
{
  dispatcher()->queue_send(DriverProtocol::packetCameraList() );
  wait_for_processed(DriverProtocol::CameraListReply);
  return d->cameras;;
}

CameraPtr RemoteDriver::existing_running_camera() const
{
  if(! d->status.imager_running)
    return {};
  return make_shared<RemoteCamera>(QString{}, -1l, dispatcher());
}


