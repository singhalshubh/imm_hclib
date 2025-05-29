include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.pre.mak
include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.post.mak

PAPI_ROOT=/usr/local/pace-apps/manual/packages/papi/7.0.1/usr/local

SRUN ?= oshrun

TARGETS= clean production_2D


all: $(TARGETS)

%: %.cpp
	$(CXX) -g -O3 -std=c++11 -DUSE_SHMEM=1 -ftrapv -I$(TRNG_ROOT)/include -L$(TRNG_ROOT)/build/trng -I./agl $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -lconvey -llibgetput -lhclib_bale_actor -ltrng4 -lm

clean:
	rm -f $(TARGETS) inf.txt time.txt