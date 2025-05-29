class Mapper {
    public:
        virtual VERTEX to_global(VERTEX v_local) const = 0;
        virtual VERTEX to_local(VERTEX v) const = 0;
        virtual VERTEX to_host(VERTEX v) const = 0;
};

bool is_even(VERTEX x) {
  return !(x&1);
}

bool is_pow2(VERTEX x) {
  return !(x & (x - 1));
}

VERTEX int_log2(VERTEX x) {
  int log_x = 0;
  while ((1l << log_x) < x) {
    log_x++;
  }
  return log_x;
}

VERTEX roundup_divide(VERTEX numerator, VERTEX denominator) {
  return (numerator + denominator - 1) / denominator;
}

class CyclicMapper : public Mapper {
    public:
        CyclicMapper() : log_num_pes_(int_log2(THREADS)),
            mask_(THREADS - 1), pow2_(is_pow2(THREADS)) {}

        virtual VERTEX to_global(VERTEX v_local) const {
            if (pow2_)
            return (v_local << log_num_pes_) + MYTHREAD;
            else
            return v_local * THREADS + MYTHREAD;
        }

        virtual VERTEX to_local(VERTEX v) const {
            if (pow2_)
            return v >> log_num_pes_;
            else
            return v / THREADS;
        }

        virtual VERTEX to_host(VERTEX v) const {
            if (pow2_)
            return v & mask_;
            else
            return v % THREADS;
        }

    protected:
        const bool pow2_;
        const VERTEX log_num_pes_;
        const VERTEX mask_;
};

class RangeMapper : public Mapper {
    public:
        RangeMapper(VERTEX global_num_nodes_) :
            v_per_node_(roundup_divide(global_num_nodes_, THREADS)),
            pow2_(is_pow2(v_per_node_)),
            mask_(v_per_node_ - 1), log_v_per_node_(int_log2(v_per_node_)) {}

        VERTEX to_global(VERTEX v_local) const {
        if (pow2_)
            return (MYTHREAD << log_v_per_node_) + v_local;
        else
            return MYTHREAD * v_per_node_ + v_local;
        }

        VERTEX to_local(VERTEX v) const {
        if (pow2_)
            return v & mask_;
        else
            return v % v_per_node_;
        }

        VERTEX to_host(VERTEX v) const {
        if (pow2_)
            return v >> log_v_per_node_;
        else
            return v / v_per_node_;
        }

    private:
        const VERTEX v_per_node_;
        const bool pow2_;
        const VERTEX log_v_per_node_;
        const VERTEX mask_;
};


// XOR reduction into needed number of bits
class XORMapper : public CyclicMapper {
    public:
       XORMapper(VERTEX global_num_nodes_) : CyclicMapper() {
            assert(pow2_);
            int bits_left = int_log2(global_num_nodes_);
            if (log_num_pes_ == 0) {
                shamts_.push_back(bits_left);
            } 
            else {
                while (bits_left > log_num_pes_) {
                    if (bits_left / 2 >= log_num_pes_) {
                        shamts_.push_back(bits_left/2);
                        bits_left = roundup_divide(bits_left, 2);
                    } 
                    else {
                        int next_shamt = std::min(log_num_pes_, bits_left - log_num_pes_);
                        bits_left -= next_shamt;
                        shamts_.push_back(next_shamt);
                    }
                }
            }
        }

    virtual VERTEX to_global(VERTEX v_local) const {
        VERTEX lower = to_host(v_local << log_num_pes_);
        lower ^= MYTHREAD;
        // handle smeared bits from overlapped final shift and XOR
        if (shamts_.back() < log_num_pes_) {
            VERTEX fixup_mask = 1l << (log_num_pes_ - 1);
            VERTEX limit = 1l << shamts_.back();
            for (fixup_mask; fixup_mask >= limit; fixup_mask >>= 1) {
                lower ^= (lower & fixup_mask) >> shamts_.back();
            }
        }
        return (v_local << log_num_pes_) | (lower & mask_);
    }

    virtual VERTEX to_local(VERTEX v) const {
        return v >> log_num_pes_;
    }

    virtual VERTEX to_host(VERTEX v) const {
        for (int shamt : shamts_) {
            v ^= (v >> shamt);
        }
        return v & mask_;
    }

    protected:
        std::vector<int> shamts_;
};


// cyclic but flip alternatively
class SnakeMapper : public CyclicMapper {
 public:
  using CyclicMapper::CyclicMapper;

  VERTEX to_global(VERTEX v_local) const {
    if (is_even(v_local)) {
      if (pow2_)
        return (v_local << log_num_pes_) + MYTHREAD;
      else
        return v_local * THREADS + MYTHREAD;
    } else {
      if (pow2_)
        return (v_local << log_num_pes_) + ((THREADS - MYTHREAD - 1) &mask_);
      else
        return v_local*THREADS + (THREADS - MYTHREAD - 1) % THREADS;
    }
  }

  VERTEX to_local(VERTEX v) const {
    if (pow2_)
      return v >> log_num_pes_;
    else
      return v / THREADS;
  }

  VERTEX to_host(VERTEX v) const {
    if (pow2_) {
      VERTEX round = v >> log_num_pes_;
      if (is_even(round))
        return v & mask_;
      else
        return (THREADS - (v&mask_) - 1) & mask_;
    } else {
      VERTEX round = v / THREADS;
      if (is_even(round))
        return v % THREADS;
      else
        return (THREADS - (v%THREADS) - 1) % THREADS;
    }
  }
};


// cyclic but rotate by 1
class RotationMapper : public CyclicMapper {
 public:
  using CyclicMapper::CyclicMapper;

  VERTEX to_global(VERTEX v_local) const {
    if (pow2_) {
      VERTEX offset = v_local & mask_;
      return (v_local << log_num_pes_) + ((MYTHREAD - offset + THREADS) & mask_);
    } else {
      VERTEX offset = v_local % THREADS;
      return v_local * THREADS + (MYTHREAD - offset + THREADS) % THREADS;
    }
  }

  VERTEX to_local(VERTEX v) const {
    if (pow2_)
      return v >> log_num_pes_;
    else
      return v / THREADS;
  }

  VERTEX to_host(VERTEX v) const {
    if (pow2_) {
      VERTEX offset = v >> log_num_pes_;
      return (v + offset) & mask_;
    } else {
      VERTEX offset = v / THREADS;
      return (v + offset) % THREADS;
    }
  }
};


// cyclic but flip and move by 1 alternatively
class SnakeRotationMapper : public CyclicMapper {
 public:
  using CyclicMapper::CyclicMapper;

  VERTEX to_global(VERTEX v_local) const {
    if (pow2_) {
      VERTEX offset = (v_local >> 1) & mask_;
      if (is_even(v_local)) {
        return (v_local << log_num_pes_) + ((MYTHREAD - offset + THREADS) & mask_);
      } else {
        return (v_local << log_num_pes_) +
               ((THREADS + offset - (MYTHREAD&mask_) - 1) & mask_);
      }
    } else {
      VERTEX offset = (v_local/2) % THREADS;
      if (is_even(v_local)) {
        return v_local*THREADS + (MYTHREAD - offset + THREADS) % THREADS;
      } else {
        return v_local*THREADS + (THREADS + offset - (MYTHREAD%THREADS) - 1)
               % THREADS;
      }
    }
  }

  VERTEX to_local(VERTEX v) const {
    if (pow2_)
      return v >> log_num_pes_;
    else
      return v / THREADS;
  }

  VERTEX to_host(VERTEX v) const {
    if (pow2_) {
      VERTEX round = v >> log_num_pes_;
      VERTEX offset = (round >> 1) & mask_;
      if (is_even(round)) {
        return (offset + v) & mask_;
      } else {
        return (THREADS + offset - (v&mask_) - 1) & mask_;
      }
    } else {
      VERTEX round = v / THREADS;
      VERTEX offset = (round/2) % THREADS;
      if (is_even(round)) {
        return (offset + v) % THREADS;
      } else {
        return (THREADS + offset - (v%THREADS) - 1) % THREADS;
      }
    }
  }
};
