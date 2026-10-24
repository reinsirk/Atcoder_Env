// SIRKIT_API_BEGIN
// [BinomialOptions]
// 事前計算と、積による代替計算の上限。
// table_limit: 各テーブルの最大添字（既定5'100'000）。0ならmax_n=0で構築する。
// max_multiplicative_steps: 1照会の積の回数上限（既定1'000'000）。超過はlength_error。
// SIRKIT_API_END
#ifndef SIRKIT_BINOMIAL_OPTIONS_DEFINED
#define SIRKIT_BINOMIAL_OPTIONS_DEFINED
struct BinomialOptions
{
    std::uint32_t table_limit = 5'100'000;

    std::uint64_t max_multiplicative_steps = 1'000'000;
};
#endif
