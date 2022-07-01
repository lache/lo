package rating

import (
	"math"
	"log"
)

// some constants copied from https://github.com/golang/go/blob/master/src/math/bits.go
const (
	shift = 64 - 11 - 1
	bias  = 1023
	mask  = 0x7FF
)

// Round returns the nearest integer, rounding half away from zero.
// This function is available natively in Go 1.10
//
// Special cases are:
//	Round(±0) = ±0
//	Round(±Inf) = ±Inf
//	Round(NaN) = NaN
func Round(x float64) float64 {
	// Round is a faster implementation of:
	//
	// func Round(x float64) float64 {
	//   t := Trunc(x)
	//   if Abs(x-t) >= 0.5 {
	//     return t + Copysign(1, x)
	//   }
	//   return t
	// }
	const (
		signMask = 1 << 63
		fracMask = 1<<shift - 1
		half     = 1 << (shift - 1)
		one      = bias << shift
	)

	bits := math.Float64bits(x)
	e := uint(bits>>shift) & mask
	if e < bias {
		// Round abs(x) < 1 including denormals.
		bits &= signMask // +-0
		if e == bias-1 {
			bits |= one // +-1
		}
	} else if e < bias+shift {
		// Round any abs(x) >= 1 containing a fractional component [0,1).
		//
		// Numbers with larger exponents are returned unchanged since they
		// must be either an integer, infinity, or NaN.
		e -= bias
		bits += half >> e
		bits &^= fracMask >> e
	}
	return math.Float64frombits(bits)
}

func CalculateNewRating(old1 int, old2 int, winner int) (int, int) {
	e1 := 1.0 / (1.0 + math.Pow(10.0, (float64)(old2 - old1)/400))
	e2 := 1.0 / (1.0 + math.Pow(10.0, (float64)(old1 - old2)/400))
	K := 32
	var s1, s2 float64
	if winner == 0 {
		s1, s2 = 0.5, 0.5
	} else if winner == 1 {
		s1, s2 = 1.0, 0.0
	} else if winner == 2 {
		s1, s2 = 0.0, 1.0
	} else {
		log.Printf("invalid winner %v", winner)
	}
	new1 := float64(old1) + float64(K) * (s1 - e1)
	new2 := float64(old2) + float64(K) * (s2 - e2)
	return int(Round(new1)), int(Round(new2))
}
