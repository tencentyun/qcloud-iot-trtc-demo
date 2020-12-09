package recordsdk
// #cgo CFLAGS: -I../include
// #cgo CXXFLAGS: -I../include -std=c++11
// #cgo LDFLAGS: -L${SRCDIR}/../lib -lTRTCEngine -lz -ldl -lm
import "C"
const (
    recVersion   = "6.5.26"
    mixerVersion = "0.0.15"
)
