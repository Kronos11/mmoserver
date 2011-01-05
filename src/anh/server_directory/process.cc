/*
 This file is part of MMOServer. For more information, visit http://swganh.com
 
 Copyright (c) 2006 - 2010 The SWG:ANH Team

 MMOServer is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MMOServer is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MMOServer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "anh/server_directory/process.h"

using namespace anh::server_directory;

Process::Process(uint32_t id,
                 uint32_t cluster_id,
                 const std::string& type,
                 const std::string& version,
                 const std::string& address,
                 uint16_t tcp_port,
                 uint16_t udp_port,
                 StatusType status,
                 const std::string& last_pulse)
    : id_(id)
    , cluster_id_(cluster_id)
    , type_(type)
    , version_(version)
    , address_(address)
    , tcp_port_(tcp_port)
    , udp_port_(udp_port)
    , status_(status)
    , last_pulse_(last_pulse)
{}

Process::~Process() {}

Process::Process(const Process& other) {
    id_ = other.id_;
    cluster_id_ = other.cluster_id_;
    type_ = other.type_;
    version_ = other.version_;
    address_ = other.address_;
    tcp_port_ = other.tcp_port_;
    udp_port_ = other.udp_port_;
    status_ = other.status_;
    last_pulse_ = other.last_pulse_;
}

Process::Process(Process&& other) {
    id_ = other.id_;
    cluster_id_ = other.cluster_id_;
    type_ = std::move(other.type_);
    version_ = std::move(other.version_);
    address_ = std::move(other.address_);
    tcp_port_ = other.tcp_port_;
    udp_port_ = other.udp_port_;
    status_ = other.status_;
    last_pulse_ = std::move(other.last_pulse_);
}

void Process::swap(Process& other) {
    std::swap(other.id_, id_);
    std::swap(other.cluster_id_, cluster_id_);
    std::swap(other.type_, type_);
    std::swap(other.version_, version_);
    std::swap(other.address_, address_);
    std::swap(other.tcp_port_, tcp_port_);
    std::swap(other.udp_port_, udp_port_);
    std::swap(other.status_, status_);
    std::swap(other.last_pulse_, last_pulse_);
}

Process& Process::operator=(Process other) {
    other.swap(*this);
    return *this;
}

uint32_t Process::id() const {
    return id_;
}

uint32_t Process::cluster_id() const {
    return cluster_id_;
}

const std::string& Process::type() const {
    return type_;
}

const std::string& Process::version() const {
    return version_;
}

const std::string& Process::address() const {
    return address_;
}

uint16_t Process::tcp_port() const {
    return tcp_port_;
}

uint16_t Process::udp_port() const {
    return udp_port_;
}

Process::StatusType Process::status() const {
    return status_;
}

const std::string& Process::last_pulse() const {
    return last_pulse_;
}