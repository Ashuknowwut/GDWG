#include "gdwg_graph.h"

#include <catch2/catch.hpp>

TEST_CASE("Constructor work as expected") {
	SECTION("default constructor") {
		auto g = gdwg::graph<std::string, int>{};
		CHECK(g.empty());
	}
	SECTION("constructor with initializer list") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		CHECK(g.nodes().size() == 3);
	}
	SECTION("constructor with input iterator") {
		auto v = std::vector<std::string>{"A", "B", "C", "D"};
		auto g = gdwg::graph<std::string, int>{v.begin(), v.end()};
		CHECK(g.nodes().size() == 4);
	}
	SECTION("move and move-assign constructor") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		auto move_g = gdwg::graph<std::string, int>{std::move(g)};
		CHECK(g.empty());
		CHECK(move_g.nodes().size() == 3);
		auto move_assign_g = std::move(move_g);
		CHECK(move_g.empty());
		CHECK(move_assign_g.nodes().size() == 3);
	}
	SECTION("copy and copy-assign constructor") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		auto copy_g = gdwg::graph<std::string, int>{g};
		CHECK(g.nodes() == copy_g.nodes());
		auto copy_assign_g = g;
		CHECK(copy_assign_g.nodes() == g.nodes());
	}
}

TEST_CASE("Edge member functions work as expected") {
	SECTION("print_edge() & is_weighted() & get_weight() & get_nodes()") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		g.insert_edge("A", "B", 3);
		g.insert_edge("B", "C");
		auto edge_AB = g.edges("A", "B")[0];
		auto edge_BC = g.edges("B", "C")[0];
		// check print_edge()
		CHECK(edge_AB->print_edge() == "A -> B | W | 3");
		CHECK(edge_BC->print_edge() == "B -> C | U");
		// check is_weighted()
		CHECK(edge_AB->is_weighted());
		CHECK(not edge_BC->is_weighted());
		// check get_weight()
		CHECK(edge_AB->get_weight() == 3);
		CHECK(edge_BC->get_weight() == std::nullopt);
		// check get_nodes()
		CHECK(edge_AB->get_nodes().first == "A");
		CHECK(edge_BC->get_nodes().second == "C");
	}
}

TEST_CASE("Modifiers work as expected") {
	SECTION("insert_node") {
		auto g = gdwg::graph<int, std::string>{};
		auto n = 5;
		CHECK(g.insert_node(n));
		CHECK(not g.insert_node(n));
		CHECK(g.is_node(n));
	}
	SECTION("insert_edge") {
		auto g = gdwg::graph<int, std::string>{3, 4, 5};
		try {
			g.insert_edge(6, 7, "A");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what())
			      == "Cannot call gdwg::graph<N, E>::insert_edge when either src or dst node does not "
			         "exist");
		} catch (...) {
			CHECK(false);
		}
		CHECK(g.insert_edge(3, 4, "A"));
		CHECK(not g.insert_edge(3, 4, "A"));
		CHECK(g.edges(3, 4).size() == 1);
	}
	SECTION("replace_node") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		g.insert_edge("A", "B", 3);
		g.insert_edge("B", "C", 5);
		try {
			g.replace_node("D", "T");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what()) == "Cannot call gdwg::graph<N, E>::replace_node on a node that doesn't exist");
		} catch (...) {
			CHECK(false);
		}
		CHECK(not g.replace_node("B", "C"));
		CHECK(g.replace_node("B", "T"));
		auto n = g.nodes();
		auto edge_AT = g.edges("A", "T")[0];
		auto edge_TC = g.edges("T", "C")[0];
		CHECK(std::find(n.begin(), n.end(), "T") != n.end());
		CHECK(std::find(n.begin(), n.end(), "B") == n.end());
		CHECK(edge_AT->get_nodes().second == "T");
		CHECK(edge_TC->get_nodes().first == "T");
	}
	SECTION("merge_replace_node") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C", "D"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("A", "C", 2);
		g.insert_edge("A", "D", 3);
		g.insert_edge("B", "B", 1);
		try {
			g.merge_replace_node("X", "T");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what())
			      == "Cannot call gdwg::graph<N, E>::merge_replace_node on old or new data if they don't exist in the "
			         "graph");
		} catch (...) {
			CHECK(false);
		}
		g.merge_replace_node("A", "B");
		auto edge_BB = g.edges("B", "B");
		auto edge_BC = g.edges("B", "C")[0];
		CHECK(edge_BB.size() == 1);
		CHECK(edge_BC->get_nodes().first == "B");
	}
	SECTION("erase_node") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("A", "C", 2);
		g.insert_edge("B", "B", 1);
		CHECK(g.erase_node("B"));
		CHECK(not g.erase_node("B"));
		CHECK(g.nodes().size() == 2);
	}
	SECTION("erase_edge") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C", "D"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("A", "C", 2);
		g.insert_edge("A", "D", 3);
		g.insert_edge("B", "D", 6);
		g.insert_edge("B", "B", 1);
		// erase a edge
		try {
			g.erase_edge("X", "T");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what())
			      == "Cannot call gdwg::graph<N, E>::erase_edge on src or dst if they don't exist in the graph");
		} catch (...) {
			CHECK(false);
		}
		CHECK(g.erase_edge("A", "B", 1));
		CHECK(not g.erase_edge("A", "B", 1));
		CHECK(g.edges("A", "B").empty());
		// erase using iterator
		auto it = g.find("A", "C", 2);
		it = g.erase_edge(it);
		CHECK(g.edges("A", "C").empty());
		CHECK((*it)->get_nodes() == std::pair<std::string, std::string>{"A", "D"});
		// range erase
		auto it_begin = g.find("A", "D", 3);
		auto it_end = g.find("B", "D", 6);
		auto it_res = g.erase_edge(it_begin, it_end);
		CHECK(g.edges("A", "D").empty());
		CHECK(g.edges("B", "B").empty());
		CHECK((*it_res)->get_nodes() == std::pair<std::string, std::string>{"B", "D"});
	}
	SECTION("clear") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("A", "C", 2);
		g.clear();
		CHECK(g.empty());
	}
}

TEST_CASE("Accessors work as expected") {
	SECTION("is_node") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		CHECK(g.is_node("A"));
		CHECK(not g.is_node("T"));
	}
	SECTION("empty") {
		auto g = gdwg::graph<std::string, int>{};
		CHECK(g.empty());
		g.insert_node("A");
		CHECK(not g.empty());
	}
	SECTION("is_connected") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("A", "C", 2);
		try {
			(void)g.is_connected("X", "T");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what())
			      == "Cannot call gdwg::graph<N, E>::is_connected if src or dst node don't exist in the graph");
		} catch (...) {
			CHECK(false);
		}
		CHECK(g.is_connected("A", "B"));
		CHECK(not g.is_connected("B", "C"));
	}
	SECTION("nodes") {
		auto g = gdwg::graph<std::string, int>{"A", "G", "C", "X", "B"};
		CHECK(g.nodes() == std::vector<std::string>{"A", "B", "C", "G", "X"});
	}
	SECTION("edges") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C", "D"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("B", "D");
		g.insert_edge("B", "D", 6);
		g.insert_edge("B", "D", 3);
		auto e = g.edges("B", "D");
		CHECK(e.size() == 3);
		CHECK(e[0]->get_weight() == std::nullopt);
		CHECK(e[2]->get_weight() == 6);
		CHECK(g.edges("B", "C").empty());
	}
	SECTION("find") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C", "D"};
		g.insert_edge("A", "B", 1);
		g.insert_edge("B", "D");
		g.insert_edge("B", "D", 6);
		g.insert_edge("B", "D", 3);
		auto it = g.find("B", "D", 3);
		auto it_ne = g.find("A", "A", 3);
		CHECK((*it)->get_weight() == 3);
		CHECK(it_ne == g.find("X", "T"));
	}
	SECTION("connections") {
		auto g = gdwg::graph<std::string, int>{"A", "B", "C", "S"};
		g.insert_edge("A", "S", 1);
		g.insert_edge("A", "C", 2);
		g.insert_edge("A", "B", 3);
		g.insert_edge("A", "A", 6);
		g.insert_edge("A", "C", 1);
		try {
			(void)g.connections("T");
		} catch (const std::runtime_error& e) {
			CHECK(std::string(e.what()) == "Cannot call gdwg::graph<N, E>::connections if src doesn't exist in the graph");
		} catch (...) {
			CHECK(false);
		}
		auto cA = g.connections("A");
		auto cB = g.connections("B");
		CHECK(cA == std::vector<std::string>{"A", "B", "C", "S"});
		CHECK(cB.empty());
	}
}

TEST_CASE("Comparisons") {
	SECTION("operator==") {
		auto g = gdwg::graph<std::string, int>{"A", "C", "S"};
		g.insert_edge("A", "S", 1);
		g.insert_edge("A", "C", 2);
		auto copy_g = g;
		CHECK(g == copy_g);
		auto move_assign_g = std::move(g);
		CHECK(g != move_assign_g);
	}
}

TEST_CASE("Extractor") {
	SECTION("operator<<") {
		using graph = gdwg::graph<int, int>;
		auto const v = std::vector<std::tuple<int, int, std::optional<int>>>{
		    {4, 1, -4},
		    {3, 2, 2},
		    {2, 4, std::nullopt},
		    {2, 4, 2},
		    {4, 1, std::nullopt},
		    {2, 1, 1},
		    {6, 2, 5},
		    {6, 3, 10},
		    {1, 5, -1},
		    {3, 6, -8},
		    {4, 5, 3},
		    {5, 2, std::nullopt},
		};

		auto g = graph{};
		for (const auto& [from, to, weight] : v) {
			g.insert_node(from);
			g.insert_node(to);
			if (weight.has_value()) {
				g.insert_edge(from, to, weight.value());
			}
			else {
				g.insert_edge(from, to);
			}
		}
		g.insert_node(64);

		auto out = std::ostringstream{};
		out << g;
		auto const expected_output = std::string_view(R"(
1 (
  1 -> 5 | W | -1
)
2 (
  2 -> 1 | W | 1
  2 -> 4 | U
  2 -> 4 | W | 2
)
3 (
  3 -> 2 | W | 2
  3 -> 6 | W | -8
)
4 (
  4 -> 1 | U
  4 -> 1 | W | -4
  4 -> 5 | W | 3
)
5 (
  5 -> 2 | U
)
6 (
  6 -> 2 | W | 5
  6 -> 3 | W | 10
)
64 (
)
)");
		CHECK(out.str() == expected_output);
	}
}

TEST_CASE("Iterator") {
	auto g = gdwg::graph<int, int>{};
	auto const v = std::vector<std::tuple<int, int, std::optional<int>>>{
	    {21, 14, 23},
	    {1, 12, 3},
	    {1, 21, 12},
	    {7, 21, 13},
	    {14, 14, 0},
	    {19, 21, 2},
	    {21, 31, 14},
	    {1, 7, 4},
	    {19, 1, 3},
	    {12, 19, 16},
	};
	for (const auto& [from, to, weight] : v) {
		g.insert_node(from);
		g.insert_node(to);
		if (weight.has_value()) {
			g.insert_edge(from, to, weight.value());
		}
		else {
			g.insert_edge(from, to);
		}
	}

	auto it = g.begin(), ite = g.end();
	++it;
	auto const& [f, t, w] = (*it);
	CHECK((f == 1 and t == 12 and w == 3));
	--ite;
	auto const& [fe, te, we] = (*ite);
	CHECK((fe == 21 and te == 31 and we == 14));

	auto out = std::ostringstream{};
	out << '\n';
	for (auto const& [from, to, weight] : g) {
		out << from << " -> " << to << " ";
		if (weight.value()) {
			out << "(weight " << *weight << ")\n";
		}
		else {
			out << "(no weight)\n";
		}
	}
	auto const expected_output = std::string_view(R"(
1 -> 7 (weight 4)
1 -> 12 (weight 3)
1 -> 21 (weight 12)
7 -> 21 (weight 13)
12 -> 19 (weight 16)
14 -> 14 (no weight)
19 -> 1 (weight 3)
19 -> 21 (weight 2)
21 -> 14 (weight 23)
21 -> 31 (weight 14)
)");
	CHECK(out.str() == expected_output);
}