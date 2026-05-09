// ============================================================
//  Robot physical constants (mirror of leg_config.h)
// ============================================================
export const L1 = 60.5   // coxa  (mm)
export const L2 = 120.0  // femur (mm)
export const L3 = 167.8  // tibia (mm)

export const STAND_Z = 120.0
export const STAND_X = 170.0

// Mount angles (deg) from front axis, CCW positive
export const LEGS = [
  { id: 'LF', side: 'L', mountDeg:  45 },
  { id: 'LM', side: 'L', mountDeg:  90 },
  { id: 'LB', side: 'L', mountDeg: 135 },
  { id: 'RF', side: 'R', mountDeg: -45 },
  { id: 'RM', side: 'R', mountDeg: -90 },
  { id: 'RB', side: 'R', mountDeg: -135 },
]

// Default servo offsets (deg)
export const DEFAULT_OFFSETS = {
  LF: { cx: 0, fm: 0, tb: 0 },
  LM: { cx: 0, fm: 0, tb: 0 },
  LB: { cx: 0, fm: 0, tb: 0 },
  RF: { cx: 2, fm: 0, tb: 0 },
  RM: { cx: 2, fm: 0, tb: 0 },
  RB: { cx: 2, fm: 0, tb: 0 },
}

// Gait defaults
export const DEFAULT_GAIT = {
  stepX: 15,
  liftZ: 30,
  tLiftMs: 120,
  tSwingMs: 220,
  tDownMs: 120,
  trimY: 0,    // mm — dương → bẻ phải, âm → bẻ trái
}
