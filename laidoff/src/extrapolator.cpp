#include <assert.h>
#include <math.h>
#include <string.h>
#include "extrapolator.h"
#include "lwlog.h"

template<int Count, typename Type>
Extrapolator<Count, Type>::Extrapolator() {
	Type pos[Count];
	memset(pos, 0, sizeof(pos));
	Reset(0, 0, pos);
}

template<int Count, typename Type>
Extrapolator<Count, Type>::~Extrapolator() {
}

template<int Count, typename Type>
bool Extrapolator<Count, Type>::AddSample(double packetTime, double curTime, Type const pos[Count]) {
	//  The best guess I can make for velocity is the difference between 
	//  this sample and the last registered sample.
	Type vel[Count];
	if (fabs(packetTime - lastPacketTime_) > 1e-4) {
		double dt = 1.0 / (packetTime - lastPacketTime_);
		for (int i = 0; i < Count; ++i) {
			vel[i] = (Type)((pos[i] - lastPacketPos_[i]) * dt);
		}
	} else {
		memset(vel, 0, sizeof(vel));
	}
	return AddSample(packetTime, curTime, pos, vel);
}

template<int Count, typename Type>
bool Extrapolator<Count, Type>::AddSample(double packetTime, double curTime, Type const pos[Count], Type const vel[Count]) {
	if (!Estimates(packetTime, curTime)) {
		return false;
	}
	memcpy(lastPacketPos_, pos, sizeof(lastPacketPos_));
	lastPacketTime_ = packetTime;
	ReadPosition(curTime, snapPos_);
	aimTime_ = curTime + updateTime_;
	double dt = aimTime_ - packetTime;
	snapTime_ = curTime;
	for (int i = 0; i < Count; ++i) {
		aimPos_[i] = (Type)(pos[i] + vel[i] * dt);
	}
	//  I now have two positions and two times:
	//  aimPos_ / aimTime_
	//  snapPos_ / snapTime_
	//  I must generate the interpolation velocity based on these two samples.
	//  However, if aimTime_ is the same as snapTime_, I'm in trouble. In that 
	//  case, use the supplied velocity.
	if (fabs(aimTime_ - snapTime_) < 1e-4) {
		for (int i = 0; i < Count; ++i) {
			snapVel_[i] = vel[i];
		}
	} else {
		double dt2 = 1.0 / (aimTime_ - snapTime_);
		for (int i = 0; i < Count; ++i) {
			snapVel_[i] = (Type)((aimPos_[i] - snapPos_[i]) * dt2);
		}
	}
	return true;
}

template<int Count, typename Type>
void Extrapolator<Count, Type>::Reset(double packetTime, double curTime, Type const pos[Count]) {
	Type vel[Count];
	memset(vel, 0, sizeof(vel));
	Reset(packetTime, curTime, pos, vel);
}

template<int Count, typename Type>
void Extrapolator<Count, Type>::Reset(double packetTime, double curTime, Type const pos[Count], Type const vel[Count]) {
	assert(packetTime <= curTime);
	lastPacketTime_ = packetTime;
	memcpy(lastPacketPos_, pos, sizeof(lastPacketPos_));
	snapTime_ = curTime;
	memcpy(snapPos_, pos, sizeof(snapPos_));
	updateTime_ = curTime - packetTime;
	latency_ = updateTime_;
	aimTime_ = curTime + updateTime_;
	memcpy(snapVel_, vel, sizeof(snapVel_));
	for (int i = 0; i < Count; ++i) {
		aimPos_[i] = (Type)(snapPos_[i] + snapVel_[i] * updateTime_);
	}
}

template<int Count, typename Type>
bool Extrapolator<Count, Type>::ReadPosition(double forTime, Type oPos[Count]) const {
	Type vel[Count];
	return ReadPosition(forTime, oPos, vel);
}

template<int Count, typename Type>
bool Extrapolator<Count, Type>::ReadPosition(double forTime, Type oPos[Count], Type oVel[Count]) const {
	bool ok = true;

	//  asking for something before the allowable time?
	if (forTime < snapTime_) {
		forTime = snapTime_;
		ok = false;
	}

	//  asking for something very far in the future?
	double maxRange = aimTime_ + updateTime_;
	if (forTime > maxRange) {
		forTime = maxRange;
		ok = false;
	}

	//  calculate the interpolated position
	for (int i = 0; i < Count; ++i) {
		oVel[i] = snapVel_[i];
		oPos[i] = (Type)(snapPos_[i] + oVel[i] * (forTime - snapTime_));
	}
	if (!ok) {
		memset(oVel, 0, sizeof(Type) * Count);
	}

	return ok;
}

template<int Count, typename Type>
double Extrapolator<Count, Type>::EstimateLatency() const {
	return latency_;
}

template<int Count, typename Type>
double Extrapolator<Count, Type>::EstimateUpdateTime() const {
	return updateTime_;
}


template<int Count, typename Type>
bool Extrapolator<Count, Type>::Estimates(double packet, double cur) {
	if (packet <= lastPacketTime_) {
		return false;
	}

	//  The theory is that, if latency increases, quickly 
	//  compensate for it, but if latency decreases, be a 
	//  little more resilient; this is intended to compensate 
	//  for jittery delivery.
	double lat = cur - packet;
	if (lat < 0) lat = 0;
	if (lat > latency_) {
		latency_ = (latency_ + lat) * 0.5;
	} else {
		latency_ = (latency_ * 7 + lat) * 0.125;
	}

	//  Do the same running average for update time.
	//  Again, the theory is that a lossy connection wants 
	//  an average of a higher update time.
	double tick = packet - lastPacketTime_;
	if (tick > updateTime_) {
		updateTime_ = (updateTime_ + tick) * 0.5;
	} else {
		updateTime_ = (updateTime_ * 7 + tick) * 0.125;
	}

	return true;
}

/////////////////////////

typedef Extrapolator<5, float> Vec4Extrapolator;

extern "C" void* vec4_extrapolator_new() {
	//LOGI("vec4_extrapolator_new");
	return new Vec4Extrapolator();
}

extern "C" void vec4_extrapolator_destroy(void** self_p) {
	auto self = *reinterpret_cast<Vec4Extrapolator**>(self_p);
	delete(self);
	*self_p = 0;
}

extern "C" void vec4_extrapolator_reset(void* s, double packet_time, double now, float x, float y, float z,
	float dx, float dy) {
	//LOGI("vec4_extrapolator_reset");
	auto self = reinterpret_cast<Vec4Extrapolator*>(s);
	float pos[5] = { x, y, z, dx, dy };
	self->Reset(packet_time, now, pos);
}

extern "C" void vec4_extrapolator_add(void* s, double packet_time, double now, float x, float y, float z,
	float dx, float dy) {
	auto self = reinterpret_cast<Vec4Extrapolator*>(s);
	float pos[] = { x, y, z, dx, dy };
	self->AddSample(packet_time, now, pos);
}

extern "C" void vec4_extrapolator_add_stop(void* s, double packet_time, double now, float x, float y, float z,
	float dx, float dy) {
	auto self = reinterpret_cast<Vec4Extrapolator*>(s);
	float pos[] = { x, y, z, dx, dy };
	float vel[] = { 0, 0, 0, 0, 0 };
	self->AddSample(packet_time, now, pos, vel);
}

extern "C" void vec4_extrapolator_addex(void* s, double packet_time, double now, float x, float y, float z,
	float dx, float dy, float vx, float vy, float vz) {
	auto self = reinterpret_cast<Vec4Extrapolator*>(s);
	float pos[] = { x, y, z, dx, dy };
	float vel[] = { vx, vy, vz, 0, 0 };
	self->AddSample(packet_time, now, pos, vel);
}

extern "C" void vec4_extrapolator_read(const void* s, double now, float* x, float* y, float* z, float* dx, float* dy) {
	//LOGI("vec4_extrapolator_read");
	auto self = reinterpret_cast<const Vec4Extrapolator*>(s);
	float pos[5];
	self->ReadPosition(now, pos);
	*x = pos[0];
	*y = pos[1];
	*z = pos[2];
	*dx = pos[3];
	*dy = pos[4];
}
