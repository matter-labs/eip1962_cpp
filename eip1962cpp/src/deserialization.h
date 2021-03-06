#ifndef H_DESERIALIZATION
#define H_DESERIALIZATION

#include "features.h"
#include "common.h"
#include "repr.h"
#include "field.h"
#include "curve.h"
#include "extension_towers/fp2.h"
#include "extension_towers/fp3.h"

// *************************** PRIMITIVE deserialization *********************** //

class Deserializer
{
    std::vector<uint8_t>::const_iterator begin;
    std::vector<uint8_t>::const_iterator const end;

public:
    Deserializer(std::vector<std::uint8_t> const &input) : begin(input.cbegin()), end(input.cend()) {}

    // Consumes a byte, throws error otherwise
    u8 byte(str &err)
    {
        if (!ended())
        {
            auto ret = *begin;
            begin++;
            return ret;
        }
        else
        {
            input_err(err);
        }
    }

    // Looks at a byte, throws error otherwise
    u8 peek_byte(str &err) const
    {
        if (!ended())
        {
            return *begin;
        }
        else
        {
            input_err(err);
        }
    }

    // Deserializes number in Big endian format with bytes.
    template <usize N>
    Repr<N> number(u8 bytes, str &err)
    {
        Repr<N> num = {0};
        read(bytes, num, err);
        return num;
    }

    // // Deserializes number in Big endian format with bytes.
    // std::vector<u64> dyn_number(u8 bytes, str &err)
    // {
    //     std::vector<u64> num;
    //     num.resize((bytes + sizeof(u64) - 1) / sizeof(u64), 0);
    //     read(bytes, num, err);
    //     return num;
    // }

    // Deserializes number in Big endian format with bytes.
    std::vector<u64> dyn_number(u8 bytes, bool check_dense, str &err)
    {
        std::vector<u64> num;
        num.resize((bytes + sizeof(u64) - 1) / sizeof(u64), 0);
        read(bytes, num, check_dense, err);
        return num;
    }

    void advance(usize bytes, str &err)
    {
        for (auto i = 0; i < bytes; i++)
        {
            byte(err);
        }
    }

    bool ended() const
    {
        return begin == end;
    }

    u32 remaining() const
    {
        return end - begin;
    }

private:
    // Deserializes number in Big endian format with bytes.
    template <class T>
    void read(u8 bytes, T &num, str &err)
    {
        for (auto i = 0; i < bytes; i++)
        {
            auto b = byte(err);
            auto j = bytes - 1 - i;
            auto at = j / sizeof(u64);
            auto off = (j - at * sizeof(u64)) * 8;
            num[at] |= ((u64)b) << off;
        }
    }

        // Deserializes number in Big endian format with bytes.
    template <class T>
    void read(u8 bytes, T &num, bool check_dense, str &err)
    {
        for (auto i = 0; i < bytes; i++)
        {
            auto b = byte(err);
            if (i == 0 && check_dense) {
                if (b == 0) {
                    input_err("top byte of scalar is zero" + err);
                }
            }
            auto j = bytes - 1 - i;
            auto at = j / sizeof(u64);
            auto off = (j - at * sizeof(u64)) * 8;
            num[at] |= ((u64)b) << off;
        }
    }
};

// True if minus
bool inline deserialize_sign(Deserializer &deserializer)
{
    auto sign = deserializer.byte("Input is not long enough to get sign encoding");
    switch (sign)
    {
    case SIGN_PLUS:
        return false;
    case SIGN_MINUS:
        return true;

    default:
        input_err("sign is not encoded properly");
    }
}

template <class E>
std::vector<u64> inline deserialize_scalar(WeierstrassCurve<E> const &wc, Deserializer &deserializer)
{
    auto scalar = deserializer.dyn_number(wc.order_len(), false, "Input is not long enough to get scalar");
    // scalar <= group_order (equality is allowed)
    // if (!greater_or_equal(wc.subgroup_order(), scalar))
    // {
    //     input_err("Group order is less or equal scalar");
    // }
    return scalar;
}

std::vector<u64> inline deserialize_loop_scalar_with_bit_limit(usize bit_limit, Deserializer &deserializer)
{
    auto const length = deserializer.byte("Input is not long enough to get scalar length");
    if (length == 0)
    {
        input_err("Loop parameter scalar length is zero");
    }
    auto const max_length_for_bits = (bit_limit + 7) / 8;
    if (length > max_length_for_bits)
    {
        input_err("Scalar is too larget for bit length");
    }
    // do check(!) for DENSE representation
    auto const num = deserializer.dyn_number(length, true, "Input is not long enough to get scalar");
    if (num_bits(num) > bit_limit)
    {
        input_err("Number of bits for scalar is too large");
    }
    return num;
}

u8 inline deserialize_pairing_curve_type(Deserializer &deserializer)
{
    auto const curve_byte = deserializer.byte("Input should be longer than curve type encoding");
    switch (curve_byte)
    {
    case BLS12:
    case BN:
    case MNT4:
    case MNT6:
        break;
    default:
        input_err("Unknown curve type");
    }
    return curve_byte;
}

TwistType inline deserialize_pairing_twist_type(Deserializer &deserializer)
{
    auto const twist_byte = deserializer.byte("Input is not long enough to get twist type");
    switch (twist_byte)
    {
    case TWIST_TYPE_D:
        return D;
    case TWIST_TYPE_M:
        return M;
    default:
        unknown_parameter_err("Unknown twist type supplied");
    }
}

bool inline deserialize_boolean(Deserializer &deserializer)
{
    auto const boolean_byte = deserializer.byte("Input is not long enough to get twist type");
    switch (boolean_byte)
    {
    case 0:
        return false;
    case 1:
        return true;
    default:
        unknown_parameter_err("Invalid boolean encoding supplied");
    }
}

// ********************* OVERLOADED deserializers of Fp and Fp2 and Fp3 *********************** //

template <usize N>
Fp<N> inline deserialize_fpM(u8 mod_byte_len, PrimeField<N> const &field, Deserializer &deserializer)
{
    auto const c0 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp_c element"), field);
    return c0;
}

template <usize N>
Fp2<N> inline deserialize_fpM(u8 mod_byte_len, FieldExtension2<N> const &field, Deserializer &deserializer)
{
    auto const c0 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp2_c0 element"), field);
    auto const c1 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp2_c1 element"), field);
    return Fp2<N>(c0, c1, field);
}

template <usize N>
Fp3<N> inline deserialize_fpM(u8 mod_byte_len, FieldExtension3<N> const &field, Deserializer &deserializer)
{
    auto const c0 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp3_c0 element"), field);
    auto const c1 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp3_c1 element"), field);
    auto const c2 = Fp<N>::from_repr(deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp3_c2 element"), field);
    return Fp3<N>(c0, c1, c2, field);
}

// *************************** SPECIAL PRIMITIVE deserialization *********************** //

template <usize N>
Repr<N> inline deserialize_modulus(u8 mod_byte_len, Deserializer &deserializer)
{
    if (deserializer.peek_byte("Input is not long enough to get modulus") == 0)
    {
        input_err("In modulus encoding highest byte is zero");
    }
    auto modulus = deserializer.number<N>(mod_byte_len, "Input is not long enough to get modulus");
    constexpr Repr<N> zero = {0};
    if (modulus == zero)
    {
        unexpected_zero_err("Modulus can not be zero");
    }
    if (is_even(modulus))
    {
        input_err("Modulus is even");
    }
    constexpr Repr<N> three = {3};
    if (modulus < three)
    {
        input_err("Modulus is less than 3");
    }
    return modulus;
}

template <class F, class C>
F inline deserialize_non_residue(u8 mod_byte_len, C const &field, u8 extension_degree, Deserializer &deserializer)
{
    F const non_residue = deserialize_fpM(mod_byte_len, field, deserializer);
    // auto x = deserializer.number<N>(mod_byte_len, "Input is not long enough to get Fp element");
    // auto const non_residue = Fp<N>::from_repr(x, field);
    if (non_residue.is_zero())
    {
        unexpected_zero_err("Fp* non-residue can not be zero");
    }

    if (!non_residue.is_non_nth_root(extension_degree))
    {
        if (!in_fuzzing()) {
            input_err("Non-residue for Fp* is actually a residue");
        }
    }

    return non_residue;
}

// ************************* CURVE deserializers ***************************** //

u8 inline deserialize_group_order_length(Deserializer &deserializer) {
    auto order_len = deserializer.byte("Input is not long enough to get group size length");
    if (order_len == 0) {
        input_err("Group order length is zero");
    }
    if (order_len > MAX_GROUP_BYTE_LEN) {
        input_err("Group order length is too large");
    }

    return order_len;
}

std::vector<u64> inline deserialize_group_order(u8 order_len, Deserializer &deserializer) {
    // check for DENSE encoding
    auto order = deserializer.dyn_number(order_len, false, "Input is not long enough to get main group order size");
    auto zero = is_zero(order);
    if (zero) {
        input_err("Group order is zero");
    }
    return order;
}

template <class F, class C>
WeierstrassCurve<F> inline deserialize_weierstrass_curve(u8 mod_byte_len, C const &field, Deserializer &deserializer, bool a_must_be_zero)
{
    F a = deserialize_fpM(mod_byte_len, field, deserializer);
    F b = deserialize_fpM(mod_byte_len, field, deserializer);

    if (a_must_be_zero && !a.is_zero())
    {
        unknown_parameter_err("A parameter must be zero");
    }

    if (b.is_zero()) {
        input_err("curve shape is not supported");
    }

    auto order_len = deserialize_group_order_length(deserializer);
    auto order = deserialize_group_order(order_len, deserializer);

    return WeierstrassCurve(a, b, order, order_len);
}

// u64 inline num_units_for_group_order(const std::vector<u64> &order)
// {
//     auto bits = num_bits(order);
//     auto limbs = (bits + 63) / 64;
//     if (limbs < NUM_GROUP_LIMBS_MIN) {
//         input_err("group order is zero");
//     } else if (limbs > NUM_GROUP_LIMBS_MAX) {
//         input_err("group order is too large");
//     }

//     return u64(limbs);
// }

u64 inline num_units_for_group_order_length(const u64 order_length)
{
    auto limbs = (order_length + 7)/8;
    if (limbs < NUM_GROUP_LIMBS_MIN) {
        input_err("group order is zero");
    } else if (limbs > NUM_GROUP_LIMBS_MAX) {
        input_err("group order is too large");
    }

    return u64(limbs);
}

template <class F, class C>
CurvePoint<F> inline deserialize_curve_point(u8 mod_byte_len, C const &field, WeierstrassCurve<F> const &wc, Deserializer &deserializer)
{
    F x = deserialize_fpM(mod_byte_len, field, deserializer);
    F y = deserialize_fpM(mod_byte_len, field, deserializer);
    auto const cp = CurvePoint(x, y);

    if (!cp.check_on_curve(wc))
    {
        if (!in_fuzzing()) {
            input_err("Point is not on curve");
        }
    }

    return cp;
}

// ********************** POINTS deserialization ******************************* //
template <usize N, class F, class C>
std::vector<std::tuple<CurvePoint<Fp<N>>, CurvePoint<F>>> inline deserialize_points(u8 mod_byte_len, C const &field, WeierstrassCurve<Fp<N>> const &g1_curve, WeierstrassCurve<F> const &g2_curve, Deserializer &deserializer)
{
    // deser (CurvePoint<Fp<N>>,CurvePoint<F>) pairs
    auto const num_pairs = deserializer.byte("Input is not long enough to get number of pairs");
    if (num_pairs == 0)
    {
        input_err("Zero pairs encoded");
    }

    std::vector<std::tuple<CurvePoint<Fp<N>>, CurvePoint<F>>> points;
    for (auto i = 0; i < num_pairs; i++)
    {
        auto const subgroup_check_g1 = deserialize_boolean(deserializer);
        auto const g1 = deserialize_curve_point<Fp<N>, PrimeField<N>>(mod_byte_len, field, g1_curve, deserializer);
        auto const subgroup_check_g2 = deserialize_boolean(deserializer);
        auto const g2 = deserialize_curve_point<F>(mod_byte_len, field, g2_curve, deserializer);

        if (subgroup_check_g1) {
            if (!g1.check_correct_subgroup(g1_curve, field))
            {
                if (!in_fuzzing()) {
                    input_err("G1 or G2 point is not in the expected subgroup");
                }
            }
        }

        if (subgroup_check_g2) {
            if (!g2.check_correct_subgroup(g2_curve, field))
            {
                if (!in_fuzzing()) {
                    input_err("G1 or G2 point is not in the expected subgroup");
                }
            }
        }

        if (!g1.is_zero() && !g2.is_zero()) {
            points.push_back(std::tuple(g1, g2));
        }
    }

    return points;
}

#endif