// shadow_tracking.h - Internal Shadow Tracking Definitions

#ifndef SHADOW_TRACKING_H
#define SHADOW_TRACKING_H

// Shadow type encoding (one byte per original byte)
enum ShadowType : unsigned char {
  UNKNOWN = 0,
  FLOAT_START = 'f',
  DOUBLE_START = 'd',
};

#endif // SHADOW_TRACKING_H
