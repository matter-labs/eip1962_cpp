#ifndef H_FP6_2
#define H_FP6_2

#include "../common.h"
// #include "../element.h"
#include "fp2.h"
#include "fp3.h"
#include "fpM2.h"
#include "../field.h"

template <usize N>
class FieldExtension2over3 : public FieldExtension3<N>
{
public:
    std::array<Fp<N>, 6> frobenius_coeffs_c1;
    bool frobenius_calculated = false;

    FieldExtension2over3(FieldExtension3<N> const &field, bool needs_frobenius) : FieldExtension3<N>(field), frobenius_coeffs_c1({Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field)})
    {
        if (needs_frobenius) {
            // NON_REDISUE**(((q^0) - 1) / 6)
            auto const f_0 = Fp<N>::one(field);

            // NON_REDISUE**(((q^1) - 1) / 6)
            auto const f_1 = calc_frobenius_factor(field.non_residue(), field.mod(), 6, "Fp6_2");

            // NON_REDISUE**(((q^2) - 1) / 6)
            auto const f_2 = Fp<N>::zero(field);

            // NON_REDISUE**(((q^3) - 1) / 6)
            auto const f_3 = calc_frobenius_factor(field.non_residue(), cbn::partial_mul<3*N>(cbn::partial_mul<2*N>(field.mod(), field.mod()), field.mod()), 6, "Fp6_2");

            auto const f_4 = Fp<N>::zero(field);
            auto const f_5 = Fp<N>::zero(field);

            std::array<Fp<N>, 6> calc_frobenius_coeffs_c1 = {f_0, f_1, f_2, f_3, f_4, f_5};
            frobenius_coeffs_c1 = calc_frobenius_coeffs_c1;

            frobenius_calculated = true;
        }
    }

    FieldExtension2over3(FieldExtension3<N> const &field, FrobeniusPrecomputation<PrimeField<N>, Fp<N>, N, 6> const &frobenius_precomputation, bool needs_frobenius) : FieldExtension3<N>(field), frobenius_coeffs_c1({Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field), Fp<N>::zero(field)})
    {
        if (needs_frobenius) {
            // NON_REDISUE**(((q^0) - 1) / 6)
            auto const f_0 = Fp<N>::one(field);

            // NON_REDISUE**(((q^1) - 1) / 6)
            auto const f_1 = frobenius_precomputation.elements[0];

            // NON_REDISUE**(((q^2) - 1) / 6)
            auto const f_2 = Fp<N>::zero(field);

            // NON_REDISUE**(((q^3) - 1) / 6)
            auto const f_3 = f_1.pow(Repr<1>{3});

            auto const f_4 = Fp<N>::zero(field);
            auto const f_5 = Fp<N>::zero(field);

            std::array<Fp<N>, 6> calc_frobenius_coeffs_c1 = {f_0, f_1, f_2, f_3, f_4, f_5};
            frobenius_coeffs_c1 = calc_frobenius_coeffs_c1;

            frobenius_calculated = true;
        }
    }

    void mul_by_nonresidue(Fp3<N> &el) const
    {
        // IMPORTANT: This only works cause the structure of extension field for Fp6_2
        // is w^2 - u = 0!
        // take an element in Fp6_2 as 2 over 3 and mutplity
        // (c0 + c1 * u)*u with u^2 - xi = 0 -> (c1*xi + c0 * u)
        auto c0 = el.c2;
        el.c2 = el.c1;
        el.c1 = el.c0;
        FieldExtension3<N>::mul_by_nonresidue(c0);
        el.c0 = c0;
    }
};

template <usize N>
class Fp6_2 : public FpM2<Fp3<N>, FieldExtension2over3<N>, Fp6_2<N>, N>
{
public:
    Fp6_2(Fp3<N> c0, Fp3<N> c1, FieldExtension2over3<N> const &field) : FpM2<Fp3<N>, FieldExtension2over3<N>, Fp6_2<N>, N>(c0, c1, field)
    {
    }

    auto operator=(Fp6_2<N> const &other)
    {
        this->c0 = other.c0;
        this->c1 = other.c1;
    }

    void frobenius_map(usize power)
    {
        assert(field->frobenius_calculated);
        if (power != 1 && power != 3)
        {
            unreachable(stringf("can not reach power %u", power));
        }
        this->c0.frobenius_map(power);
        this->c1.frobenius_map(power);
        this->c1.mul_by_fp(this->field.frobenius_coeffs_c1[power % 6]);
    }

    // ************************* ELEMENT impl ********************************* //

    template <class C>
    static Fp6_2<N> one(C const &context)
    {
        FieldExtension2over3<N> const &field = context;
        return Fp6_2<N>(Fp3<N>::one(context), Fp3<N>::zero(context), field);
    }

    template <class C>
    static Fp6_2<N> zero(C const &context)
    {
        FieldExtension2over3<N> const &field = context;
        return Fp6_2<N>(Fp3<N>::zero(context), Fp3<N>::zero(context), field);
    }

    Fp6_2<N> one() const
    {
        return Fp6_2::one(this->field);
    }

    Fp6_2<N> zero() const
    {
        return Fp6_2::zero(this->field);
    }

    Fp6_2<N> &self()
    {
        return *this;
    }

    Fp6_2<N> const &self() const
    {
        return *this;
    }

    template <usize M>
    auto pow(Repr<M> const &e) const
    {
        auto res = one();
        auto found_one = false;
        auto const base = self();

        for (auto it = RevBitIterator(e); it.before();)
        {
            auto i = *it;
            if (found_one)
            {
                res.square();
            }
            else
            {
                found_one = i;
            }

            if (i)
            {
                res.mul(base);
            }
        }

        return res;
    }

    bool operator==(Fp6_2<N> const &other) const
    {
        return this->c0 == other.c0 && this->c1 == other.c1;
    }

    bool operator!=(Fp6_2<N> const &other) const
    {
        return !(*this == other);
    }

    auto cyclotomic_exp(std::vector<u64> const &exp) const
    {
        auto res = one();
        auto self_inverse = self();
        self_inverse.conjugate();

        auto const base = self();

        auto found_nonzero = false;
        auto const naf = into_ternary_wnaf(exp);

        for (auto it = naf.crbegin(); it != naf.crend(); it++)
        {
            auto const value = *it;
            if (found_nonzero)
            {
                res.square();
            }

            if (value != 0)
            {
                found_nonzero = true;

                if (value > 0)
                {
                    res.mul(base);
                }
                else
                {
                    res.mul(self_inverse);
                }
            }
        }

        return res;
    }
};

template <usize N>
std::ostream &operator<<(std::ostream &strm, Fp6_2<N> num) {
    strm << "Fp6("<< num.c0 << " + " << num.c1 << "*v)";
    return strm;
}
#endif
