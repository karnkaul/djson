#pragma once
#include <detail/tokenizer.hpp>
#include <dumb_json/json_type.hpp>

namespace dj::detail {
struct unexpect_t {
	loc_t loc;
	std::string_view text;
	std::vector<tk_type> expected;
	tk_type obtained{};
};

class parser_t {
  public:
	struct result_t;

	parser_t(std::string_view text);

	result_t parse();

  private:
	using tk_list = std::initializer_list<tk_type>;

	enum adv_t { on = 1 << 0, un = 1 << 1 };
	inline static constexpr int adv_all = adv_t::on | adv_t::un;

	ptr<json> value();
	void advance() noexcept;
	void push_error(tk_list exp, bool adv = true);
	bool check(tk_list exp) const noexcept;
	bool expect(tk_list exp, int adv = adv_t::on) noexcept;
	template <typename T>
	T to();

	tokenizer_t m_scanner;
	std::vector<unexpect_t> m_unexpected;
	std::string_view m_text;
	token_t m_curr, m_prev;
};

struct parser_t::result_t {
	ptr<json> json_;
	std::vector<unexpect_t> unexpected;

	bool error() const noexcept { return !unexpected.empty(); }
};

template <typename T, typename... Args>
ptr<T> make(Args&&... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}
} // namespace dj::detail
