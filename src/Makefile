include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.pre.mak
include $(HCLIB_ROOT)/include/hclib.mak
include $(HCLIB_ROOT)/../modules/bale_actor/inc/hclib_bale_actor.post.mak

SRUN ?= oshrun

TARGETS= clean imm_hclib_1D imm_hclib_2D

all: $(TARGETS)

%: %.cpp
	$(CXX) -g -O3 -std=c++11 -DUSE_SHMEM=1 -I$(TRNG_ROOT)/include -L$(TRNG_ROOT)/build/trng $(HCLIB_CFLAGS) $(HCLIB_LDFLAGS) -o $@ $^ $(HCLIB_LDLIBS) -lspmat -lconvey -lexstack -llibgetput -lhclib_bale_actor -ltrng4 -lm

clean:
	rm -f $(TARGETS)
