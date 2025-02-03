#ifndef GDWG_GRAPH_H
#	define GDWG_GRAPH_H

#	include <iostream>
#	include <set>
#	include <sstream>
#	include <string>
#	include <optional>
#	include <vector>
#	include <algorithm>
#	include <functional>
#	include <iterator>
#	include <memory>
#	include <tuple>

// TODO: Make both graph and edge generic
//       ... this won't just compile
//       straight away
namespace gdwg {
	template<typename N, typename E>
	class edge {
	 public:
		virtual auto print_edge() const -> std::string = 0;
		virtual auto is_weighted() const -> bool = 0;
		virtual auto get_weight() const -> std::optional<E> = 0;
		virtual auto get_nodes() const -> std::pair<N, N> = 0;
		virtual ~edge(){};

	 private:
		// helper function for replace_node and merge_replace
		virtual void replace(int p, const N& v) noexcept = 0;
		template<typename T, typename U>
		friend class graph;
	};

	template<typename N, typename E>
	class weighted_edge : public edge<N, E> {
	 public:
		weighted_edge(const N& src, const N& dst, const E& weight)
		: src_(src)
		, dst_(dst)
		, is_weighted_{true}
		, weight_(weight) {}

		auto print_edge() const -> std::string override {
			std::ostringstream oss;
			oss << src_ << " -> " << dst_ << " | W | " << weight_;
			std::string res = oss.str();
			return res;
		}

		auto is_weighted() const -> bool override {
			return is_weighted_;
		}

		auto get_weight() const -> std::optional<E> override {
			return weight_;
		}

		auto get_nodes() const -> std::pair<N, N> override {
			return std::pair<N, N>{src_, dst_};
		}

	 private:
		N src_;
		N dst_;
		bool is_weighted_;
		E weight_;
		void replace(int p, const N& v) noexcept override {
			(p == 1 ? dst_ : src_) = v;
		}
		template<typename T, typename U>
		friend class graph;
	};

	template<typename N, typename E>
	class unweighted_edge : public edge<N, E> {
	 public:
		unweighted_edge(const N& src, const N& dst)
		: src_(src)
		, dst_(dst)
		, is_weighted_{false} {}

		auto print_edge() const -> std::string override {
			std::ostringstream oss;
			oss << src_ << " -> " << dst_ << " | U";
			std::string res = oss.str();
			return res;
		}

		auto is_weighted() const -> bool override {
			return is_weighted_;
		}

		auto get_weight() const -> std::optional<E> override {
			return std::nullopt;
		}

		auto get_nodes() const -> std::pair<N, N> override {
			return std::pair<N, N>{src_, dst_};
		}

	 private:
		N src_;
		N dst_;
		bool is_weighted_;
		void replace(int p, const N& v) noexcept override {
			(p == 1 ? dst_ : src_) = v;
		}
		template<typename T, typename U>
		friend class graph;
	};

	template<typename N, typename E>
	class graph {
		class iterator;

	 public:
		using edge = std::shared_ptr<gdwg::edge<N, E>>;
		using edge_iterator = std::vector<edge>::const_iterator;

		// constructors
		graph()
		: nodes_{}
		, edges_{} {}

		graph(std::initializer_list<N> il)
		: nodes_{std::set<N>(il.begin(), il.end())}
		, edges_{} {}

		template<typename InputIt>
		graph(InputIt first, InputIt last)
		: nodes_{std::set<N>(first, last)}
		, edges_{} {}

		// move
		graph(graph&& other) noexcept
		: nodes_{std::move(other.nodes_)}
		, edges_{std::move(other.edges_)} {}

		auto operator=(graph&& other) noexcept -> graph& {
			nodes_ = std::move(other.nodes_);
			edges_ = std::move(other.edges_);
			return *this;
		}

		// copy
		graph(graph const& other)
		: nodes_{other.nodes_}
		, edges_{other.edges_} {}

		auto operator=(graph const& other) -> graph& {
			nodes_ = other.nodes_;
			edges_ = other.edges_;
			return *this;
		}

		~graph() = default;

		// modifiers
		auto insert_node(N const& value) noexcept -> bool {
			if (nodes_.contains(value)) {
				return false;
			}
			nodes_.insert(value);
			return true;
		}

		auto insert_edge(N const& src, N const& dst, std::optional<E> weight = std::nullopt) -> bool {
			if (is_node(src) and is_node(dst)) {
				if (find(src, dst, weight) == edges_.end()) {
					if (weight == std::nullopt) {
						edge new_edge = std::make_shared<unweighted_edge<N, E>>(src, dst);
						edges_.push_back(new_edge);
					}
					else {
						edge new_edge = std::make_shared<weighted_edge<N, E>>(src, dst, weight.value());
						edges_.push_back(new_edge);
					}
					sortedges();
					return true;
				}
				return false;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::insert_edge when either src or dst node does not "
			                        "exist"};
			throw std::runtime_error{emsg};
		}

		auto replace_node(N const& old_data, N const& new_data) -> bool {
			if (is_node(old_data)) {
				if (not is_node(new_data)) {
					nodes_.erase(old_data);
					nodes_.insert(new_data);
					for (auto const& e : edges_) {
						auto strpair = e->get_nodes();
						if (strpair.first == old_data) {
							e->replace(0, new_data);
						}
						if (strpair.second == old_data) {
							e->replace(1, new_data);
						}
					}
					sortedges();
					return true;
				}
				return false;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::replace_node on a node that doesn't exist"};
			throw std::runtime_error{emsg};
		}

		auto merge_replace_node(N const& old_data, N const& new_data) -> void {
			if (is_node(old_data) and is_node(new_data)) {
				for (auto e = edges_.begin(); e != edges_.end();) {
					auto strpair = (*e)->get_nodes();
					if (strpair.first == old_data) {
						(*e)->replace(0, new_data);
					}
					if (strpair.second == old_data) {
						(*e)->replace(1, new_data);
					}
					strpair = (*e)->get_nodes();
					auto first_it = find(strpair.first, strpair.second, (*e)->get_weight());
					if (first_it != e) {
						e = edges_.erase(e);
					}
					else {
						++e;
					}
				}
				sortedges();
			}
			else {
				auto emsg = std::string{"Cannot call gdwg::graph<N, E>::merge_replace_node on old or new data if they "
				                        "don't exist in the graph"};
				throw std::runtime_error{emsg};
			}
		}

		auto erase_node(N const& value) noexcept -> bool {
			if (is_node(value)) {
				for (auto e = edges_.begin(); e != edges_.end();) {
					auto strpair = (*e)->get_nodes();
					if (strpair.first == value or strpair.second == value) {
						e = edges_.erase(e);
					}
					else {
						++e;
					}
				}
				nodes_.erase(value);
				sortedges();
				return true;
			}
			return false;
		}

		auto erase_edge(N const& src, N const& dst, std::optional<E> weight = std::nullopt) -> bool {
			if (is_node(src) and is_node(dst)) {
				auto target_it = find(src, dst, weight);
				if (target_it != edges_.end()) {
					edges_.erase(target_it);
					sortedges();
					return true;
				}
				return false;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::erase_edge on src or dst if they don't exist in "
			                        "the graph"};
			throw std::runtime_error{emsg};
		}

		auto erase_edge(edge_iterator i) noexcept -> edge_iterator {
			auto res = edges_.erase(i);
			sortedges();
			return res;
		}

		auto erase_edge(edge_iterator i, edge_iterator s) noexcept -> edge_iterator {
			auto res = edges_.erase(i, s);
			sortedges();
			return res;
		}

		auto clear() noexcept -> void {
			nodes_ = std::set<N>{};
			edges_ = std::vector<edge>{};
		}

		// accessors
		[[nodiscard]] auto is_node(N const& value) const noexcept -> bool {
			return nodes_.contains(value);
		}

		[[nodiscard]] auto empty() const noexcept -> bool {
			return nodes_.empty();
		}

		[[nodiscard]] auto is_connected(N const& src, N const& dst) const -> bool {
			if (is_node(src) and is_node(dst)) {
				for (auto const& e : edges_) {
					auto strpair = e->get_nodes();
					if (strpair.first == src and strpair.second == dst) {
						return true;
					}
				}
				return false;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::is_connected if src or dst node don't exist in the "
			                        "graph"};
			throw std::runtime_error{emsg};
		}

		[[nodiscard]] auto nodes() const noexcept -> std::vector<N> {
			auto res = std::vector<N>(nodes_.begin(), nodes_.end());
			std::sort(res.begin(), res.end());
			return res;
		}

		[[nodiscard]] auto edges(N const& src, N const& dst) const -> std::vector<edge> {
			if (is_node(src) and is_node(dst)) {
				auto res = std::vector<edge>{};
				std::function<bool(edge const&, edge const&)> sort_weight = [](edge const& a, edge const& b) {
					return a->get_weight() < b->get_weight();
				};
				for (auto const& e : edges_) {
					auto strpair = e->get_nodes();
					if (strpair.first == src and strpair.second == dst) {
						res.push_back(e);
					}
				}
				std::sort(res.begin(), res.end(), sort_weight);
				return res;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::edges if src or dst node don't exist in the graph"};
			throw std::runtime_error{emsg};
		}

		[[nodiscard]] auto find(N const& src, N const& dst, std::optional<E> weight = std::nullopt) const noexcept
		    -> edge_iterator {
			for (auto e = edges_.begin(); e != edges_.end(); ++e) {
				auto strpair = (*e)->get_nodes();
				if (strpair.first == src and strpair.second == dst and (*e)->get_weight() == weight) {
					return e;
				}
			}
			return edges_.end();
		}

		[[nodiscard]] auto connections(N const& src) const -> std::vector<N> {
			if (is_node(src)) {
				auto res = std::vector<N>{};
				for (auto const& e : edges_) {
					auto strpair = e->get_nodes();
					auto dst = strpair.second;
					if (strpair.first == src and std::find(res.begin(), res.end(), dst) == res.end()) {
						res.push_back(dst);
					}
				}
				std::sort(res.begin(), res.end());
				return res;
			}
			auto emsg = std::string{"Cannot call gdwg::graph<N, E>::connections if src doesn't exist in the graph"};
			throw std::runtime_error{emsg};
		}

		// Iterator Access
		[[nodiscard]] auto begin() const -> iterator {
			return iterator(edges_.begin());
		}

		[[nodiscard]] auto end() const -> iterator {
			return iterator(edges_.end());
		}

		// Comparisons
		[[nodiscard]] auto operator==(graph const& other) const noexcept -> bool {
			auto other_n = other.nodes_;
			auto other_e = other.edges_;
			if (other_n.size() == nodes_.size() and other_e.size() == edges_.size()) {
				for (auto const& n : other_n) {
					if (std::find(nodes_.begin(), nodes_.end(), n) == nodes_.end()) {
						return false;
					}
				}
				for (auto const& e : other_e) {
					auto strpair = e->get_nodes();
					if (find(strpair.first, strpair.second, e->get_weight()) == edges_.end()) {
						return false;
					}
				}
			}
			else {
				return false;
			}
			return true;
		}

		// Extractor
		friend auto operator<<(std::ostream& os, graph const& g) noexcept -> std::ostream& {
			std::ostringstream oss;
			auto nodes = g.nodes();
			oss << "\n";
			for (auto const& n : nodes) {
				oss << n << " (\n";
				auto conn = g.connections(n);
				for (auto const& cn : conn) {
					auto cn_edges = g.edges(n, cn);
					for (auto const& e : cn_edges) {
						auto temp_str = e->print_edge();
						oss << "  " << temp_str << "\n";
					}
				}
				oss << ")\n";
			}
			auto res = oss.str();
			os << res;
			return os;
		}

	 private:
		std::set<N> nodes_;
		std::vector<edge> edges_;

		// helper function for keep the order of edges stored in graph
		void sortedges() noexcept {
			std::function<bool(edge const&, edge const&)> sort_weight = [](edge const& a, edge const& b) {
				auto node_a = a->get_nodes(), node_b = b->get_nodes();
				if (node_a.first != node_b.first) {
					return node_a.first < node_b.first;
				}
				if (node_a.second != node_b.second) {
					return node_a.second < node_b.second;
				}
				return a->get_weight() < b->get_weight();
			};
			std::sort(edges_.begin(), edges_.end(), sort_weight);
		}
	};

	template<typename N, typename E>
	class graph<N, E>::iterator {
	 public:
		using value_type = struct {
			N from;
			N to;
			std::optional<E> weight;
		};
		using reference = value_type;
		using pointer = void;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;

		// Iterator constructor
		iterator() = default;

		// Iterator source
		auto operator*() const noexcept -> reference {
			auto strpair = (*curr_)->get_nodes();
			auto weight = (*curr_)->get_weight();
			return value_type{strpair.first, strpair.second, weight};
		}

		// Iterator traversal
		auto operator++() noexcept -> iterator& {
			++curr_;
			return *this;
		}

		auto operator++(int) noexcept -> iterator {
			auto copy{*this};
			++(*this);
			return copy;
		}

		auto operator--() noexcept -> iterator& {
			--curr_;
			return *this;
		}

		auto operator--(int) noexcept -> iterator {
			auto copy{*this};
			--(*this);
			return copy;
		}

		// Iterator comparison
		auto operator==(iterator const& other) const noexcept -> bool {
			return curr_ == other.curr_;
		}

	 private:
		explicit iterator(edge_iterator curr)
		: curr_(curr) {}
		edge_iterator curr_{};
		friend class graph<N, E>;
	};
} // namespace gdwg

#endif // GDWG_GRAPH_H
// find . -name '*.cpp' -o -name '*.h' | xargs clang-format -i
