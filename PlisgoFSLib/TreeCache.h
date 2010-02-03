/*
	Copyright 2009 Eurocom Entertainment Software Limited

    This file is part of PlisgoFSLib.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “PlisgoFSLib” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    PlisgoFSLib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with PlisgoFSLib.  If not, see
	<http://www.gnu.org/licenses/>.
*/
#pragma once


template<typename TKey, typename TData,  bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
class TreeCache
{
public:
	typedef boost::unordered_map<std::wstring, TData>	FullKeyMap;

	bool GetData(const TKey& rFullKey, TData& rData, bool bReturnNearest = false);

	void SetData(const TKey& rFullKey, TData& rData);

	void PruneBranch(const TKey& rFullKey);

	void MoveBranch(const TKey& rOldFullKey, const TKey& rNewFullKey);

	void GetFullKeyMap(FullKeyMap& rFullKeyMap)
	{
		boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

		rFullKeyMap = m_FullKeyMapMap;
	}

	template<class T, class UD>
	bool ForEachChild(const TKey& rKey, T* pObj, bool (T::*Method)(const TKey& rChildKey, const TData& rData, UD& rUD), UD& rUD)
	{
		boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

		TreeNode* pTreeNode;

		if (!GetTreeNode(rKey, NONE, pTreeNode))
			return false;
		
		for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
			if (!(pObj->*Method)(it->first, it->second.Data, rUD))
				return false;

		return true;
	}

private:
	struct TreeNode;

	enum TraceTreeNodeFlags
	{
		NONE,
		NEAREST,
		CREATE
	};

	bool GetTreeNode(const TKey& rKey, TraceTreeNodeFlags eFlag, TreeNode*& rpTreeNode);
	bool GetTreeParent(const TKey& rKey, TreeNode*& rpTreeNode, TreeNode*& rpTreeParentNode, TKey& rLastSubKey);


	typedef std::map<TKey, TreeNode> ChildTreeNodes;

	struct TreeNode
	{
		TData			Data;
		ChildTreeNodes	Children;
	};


	boost::shared_mutex			m_Mutex;
	TreeNode					m_Root;
	FullKeyMap					m_FullKeyMapMap;
};


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline bool TreeCache<TKey, TData, GetNextSubKey>::GetData(const TKey& rFullKey, TData& rData, bool bReturnNearest)
{
	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	FullKeyMap::const_iterator it = m_FullKeyMapMap.find(rFullKey);

	if (it != m_FullKeyMapMap.end())
	{
		rData = it->second;
		return true;
	}
	else if (!bReturnNearest)
		return false;

	TreeNode* pTreeNode = NULL;

	if (!GetTreeNode(rFullKey, (bReturnNearest)?NEAREST:NONE, pTreeNode))
		return false;
	
	rData = pTreeNode->Data;

	return true;
}


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline void TreeCache<TKey, TData, GetNextSubKey>::SetData(const TKey& rFullKey, TData& rData)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pTreeNode = NULL;

	GetTreeNode(rFullKey, CREATE, pTreeNode);

	assert(pTreeNode); //There is no fail!
	
	pTreeNode->Data = rData;

	m_FullKeyMapMap[rFullKey] = rData;
}


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline bool TreeCache<TKey, TData, GetNextSubKey>::GetTreeNode(const TKey& rFullKey, TraceTreeNodeFlags eFlag, TreeNode*& rpTreeNode)
{
	rpTreeNode = &m_Root;

	size_t nPos = 0;
	TKey name;

	while(GetNextSubKey(rFullKey, nPos, name))
	{
		ChildTreeNodes::const_iterator it = rpTreeNode->Children.find(name);

		if (it != rpTreeNode->Children.end())
		{
			rpTreeNode = const_cast<TreeNode*>(&it->second);
		}
		else
		{
			if (eFlag == NEAREST)
				return true;

			if (eFlag != CREATE)
				return false;

			rpTreeNode = &rpTreeNode->Children[name];
		}
	}

	return true;
}


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline void TreeCache<TKey, TData, GetNextSubKey>::PruneBranch(const TKey& rFullKey)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pParentTreeNode;
	TreeNode* pTreeNode;

	TKey name;

	if (!GetTreeParent(rFullKey, pTreeNode, pParentTreeNode, name))
		return; //This path wasn't in the tree anyway

	assert(pParentTreeNode != NULL);

	pParentTreeNode->Children.erase(name);
}


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline bool TreeCache<TKey, TData, GetNextSubKey>::GetTreeParent(const TKey& rFullKey, TreeNode*& rpTreeNode, TreeNode*& rpTreeParentNode, TKey& rLastSubKey)
{
	size_t nPos = 0;

	rpTreeParentNode = NULL;
	rpTreeNode = &m_Root;

	while(GetNextSubKey(rFullKey, nPos, rLastSubKey))
	{
		ChildTreeNodes::const_iterator it = rpTreeNode->Children.find(rLastSubKey);

		if (it == rpTreeNode->Children.end())
			return false; //This path wasn't in the tree anyway
		
		rpTreeParentNode = rpTreeNode;
		rpTreeNode = const_cast<TreeNode*>(&it->second);
	}

	return true;
}


template<typename TKey, typename TData, bool (*GetNextSubKey)(const TKey& rFullKey, size_t& rnPos, TKey& rSubKey)>
inline void TreeCache<TKey, TData, GetNextSubKey>::MoveBranch(const TKey& rOldFullKey, const TKey& rNewFullKey)
{
	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pOldParentTreeNode;
	TreeNode* pOldTreeNode;

	TKey oldName;

	if (!GetTreeParent(rOldFullKey, pOldTreeNode, pOldParentTreeNode, oldName))
		return; //This path wasn't in the tree anyway

	TreeNode* pTreeNode = NULL;

	GetTreeNode(rNewFullKey, CREATE, pTreeNode);

	assert(pTreeNode != NULL);

	pTreeNode->Data		= pOldTreeNode->Data;
	pTreeNode->Children = pOldTreeNode->Children;

	assert(pOldParentTreeNode != NULL);

	pOldParentTreeNode->Children.erase(oldName);
}
