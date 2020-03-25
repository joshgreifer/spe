#pragma once
// #include "stdheaders.h"

// DAG
// using namespace std;



namespace sel {
	template<typename T>class NodeInfo {
	public:

		typedef T *pNode;
		typedef NodeInfo *pNodeInfo;
		typedef std::vector<pNodeInfo>pniList;

		pNode node;
		pniList predecessors;
		pniList successors;

		// working var for toplogical sort
		int n_predecessors;

		bool operator<(const NodeInfo& other) { return node < other.node; }
		bool operator==(const NodeInfo& other) { return node == other.node; }
		bool operator==(const pNode& other) { return node == other; }

		explicit NodeInfo(pNode pn) : node(pn)
		{
			//		printf("ctor");
		}
	};

	template<typename T>class dag
	{
		typedef T *pNode;
	public:
		typedef NodeInfo<T> NI, *pNI;
		typedef std::vector<pNI>pniList;
		typedef typename pniList::iterator pniList_iter;
		typedef typename pniList::const_iterator pniList_const_iter;

		typedef std::vector<pNode> nList;

		typedef std::pair<pNode, pNode>Edge;


		pniList nodes;
		std::vector<Edge >edges;

		dag()
		{
		}

		~dag()
		{
			pniList_iter n_iter;
			try {
				for (n_iter = nodes.begin(); n_iter != nodes.end(); ++n_iter)
					delete *n_iter;
			}
			catch (...) {}
		}

		dag& append(const dag& other)
		{
			edges.insert(edges.end(), other.edges.begin(), other.edges.end());
			edges.insert(edges.end(), other.edges.begin(), other.edges.end());
			
			return *this;
		}
		//graph(vector<Edge>& edges)
		//{
		//	for (vector<Edge>::const_iterator e = other.edges.begin(); e != other.edges.end(); ++e)
		//		add_edge(*e);
		//}

		//graph(const graph& other)
		//{		
		//	for (vector<Edge >::const_iterator e = other.edges.begin(); e != other.edges.end(); ++e)
		//		add_edge(*e);
		//}

		//graph& operator=(const graph& other)
		//{
		//	nodes.clear();
		//	
		//	for (vector<Edge >::const_iterator e = other.edges.begin(); e != other.edges.end(); ++e)
		//		add_edge(*e);
		//}

		pNI add_or_find_node(pNode node)
		{
			pNI info;
			pniList_const_iter result;
			if (!find_node_info(result, node))
				nodes.push_back(info = new NI(node));
			else
				info = *result;
			return info;
		}

		bool add_edge(pNode from, pNode to) {
			return add_edge(Edge(from, to));
		}

		bool add_edge(const Edge& e)
		{
			// don't allow same edge twice
			if (find(edges.begin(), edges.end(), e) != edges.end())
				return false;
			edges.insert(edges.end(), e);

			//NI firstInfo_find(e.first);
			//NI secondInfo_find(e.second);

			pNI pFirstInfo = add_or_find_node(e.first);
			pNI pSecondInfo = add_or_find_node(e.second);

			// now add second to first's successors
			pFirstInfo->successors.push_back(pSecondInfo);
			// now add first to second's predecessors
			pSecondInfo->predecessors.push_back(pFirstInfo);

			return true;
		}

		bool find_node_info(pniList_const_iter& result, pNode node) const
		{
			for (result = nodes.begin(); result != nodes.end(); ++result)
				if (**result == node)
					return true;
			return false;
		}

		bool get_successors(pNode node, std::vector<pNode>& vec) const
		{

			pniList_iter result;
			if (!find_node_info(result, node))
				return false;
			if ((*result)->successors.size() == 0)
				return false;
			for (pniList_iter i = (*result)->successors.begin(); i != (*result)->successors.end(); ++i) {
				vec.push_back((*i)->node);
				get_successors((*i)->node, vec);
			}
			return true;
		}

		bool get_predecessors(pNode node, std::vector<pNode>& vec) const
		{
			pniList_iter result;
			if (!find_node_info(result, node))
				return false;
			if ((*result)->predecessors.size() == 0)
				return false;
			for (pniList_iter i = (*result)->predecessors.begin(); i != (*result)->predecessors.end(); ++i) {
				vec.push_back((*i)->node);
				get_predecessors((*i)->node, vec);
			}
			return true;
		}

		bool topological_sort(std::vector<pNode>& sorted_vec) const
		{
			// I could probably do this in one pass, but Big Deal

			// initialize
			// 
			for (pniList_const_iter i = nodes.begin(); i != nodes.end(); ++i)
				(*i)->n_predecessors = (int)(*i)->predecessors.size();

			// output all nodeinfos with no predecessors
			for (pniList_const_iter i = nodes.begin(); i != nodes.end(); )
				if ((*i)->n_predecessors == 0) {
					(*i)->n_predecessors = -1;		// means it's been output
					sorted_vec.push_back((*i)->node);
					for (pniList_const_iter j = (*i)->successors.begin(); j != (*i)->successors.end(); ++j)
						--(*j)->n_predecessors;

					i = nodes.begin(); // start again;
				}
				else
					++i;

			for (pniList_const_iter i = nodes.begin(); i != nodes.end(); ++i)
				if ((*i)->n_predecessors > 0)
					return false; // cycle(s

			return true;

		}

	};

} // sel