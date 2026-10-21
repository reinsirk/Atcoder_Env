#ifndef REINSIRK_BINOMIAL_OPTIONS_DEFINED
#define REINSIRK_BINOMIAL_OPTIONS_DEFINED
// 事前計算と、積による代替計算の上限。
struct BinomialOptions
{
    // 各テーブルの最大添字。0にする場合はmax_n=0で構築する。
    std::uint32_t table_limit = 5'100'000;

    // 1回の問い合わせで許す積のループ回数。超過時は length_error。
    std::uint64_t max_multiplicative_steps = 1'000'000;
};
#endif
