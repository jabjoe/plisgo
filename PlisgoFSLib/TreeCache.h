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


template<typename WStrType, typename TData>
class TreeCache
{
public:

	typedef boost::unordered_map<WStrType, TData>	FullKeyMap;
	typedef std::map<WStrType, TData>				SubKeyMap;

	bool GetData(WStrType sFullKey, TData& rData, bool bReturnNearest = false) const;

	void SetData(WStrType sFullKey, TData& rData);

	void RemoveAndPrune(WStrType sFullKey, bool bRemoveAnyChildren = false); //Remove key value, then remove any entries in branch that have no value
	void RemoveBranch(WStrType sFullKey);

	void MoveBranch(WStrType sOldFullKey, WStrType sNewFullKey);

	void GetFullKeyMap(FullKeyMap& rsFullKeyMap) const;

	bool GetChildMap(WStrType sFullKey, SubKeyMap& rSubKeyMap, bool bIncludeEmpty = false) const;

private:
	struct TreeNode;

	bool GetTreeNode(const WStrType& rsKey, bool bReturnNearest, const TreeNode*& rpTreeNode) const;
	void GetCreateTreeNode(const WStrType& rsKey, TreeNode*& rpTreeNode);

	bool GetTreeParent(const WStrType& rsKey, TreeNode*& rpParentTreeNode, TreeNode*& rpTreeNode, WStrType& rsName);

	bool GetNextKey(const WStrType& rsKey, size_t& rnPos, WStrType& rsSubKey) const;

	void RemoveChildren(const WStrType& rsFullKey, TreeNode *pTreeNode, bool bDestroy);
	void RemoveTreeNodeFromCache(const WStrType& rsFullKey, TreeNode *pTreeNode, bool bDestroy);

	void AddTreeToFullKeyCache(const WStrType& rsFullKey, TreeNode *pTreeNode);


	typedef std::map<WStrType, TreeNode*> ChildTreeNodes;

	struct TreeNode
	{
		TreeNode() { bSet = false; }

		bool			bSet;
		TData			Data;
		ChildTreeNodes	Children;
	};


	typedef boost::unordered_map<WStrType, TreeNode*>	FullKeyNodeMap;
	typedef boost::object_pool<TreeNode>					TreeNodePool;

	mutable boost::shared_mutex		m_Mutex;
	TreeNode						m_Root;
	TreeNodePool					m_TreeNodePool;
	FullKeyNodeMap					m_FullKeyNodeMap;
};





template<typename WStrType, typename TData>
inline bool TreeCache<WStrType,TData>::GetData(WStrType sFullKey, TData& rData, bool bReturnNearest) const
{
	MakePathHashSafe<WStrType>(sFullKey);

	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	const TreeNode* pTreeNode = NULL;

	if (!GetTreeNode(sFullKey, bReturnNearest, pTreeNode))
		return false;
	
	rData = pTreeNode->Data;

	return true;
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::SetData(WStrType sFullKey, TData& rData)
{
	MakePathHashSafe(sFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pTreeNode = NULL;

	GetCreateTreeNode(sFullKey, pTreeNode);

	assert(pTreeNode != NULL); //There is no fail!
	
	pTreeNode->Data = rData;
	pTreeNode->bSet = true;
}


template<typename WStrType, typename TData>
bool TreeCache<WStrType,TData>::GetNextKey(const WStrType& rsKey, size_t& rnPos, WStrType& rsSubKey) const
{
	if (rnPos == -1)
		return false;

	size_t nNext = rsKey.find('\\',rnPos);

	if (nNext != -1)
	{
		rsSubKey.assign(rsKey.begin() + rnPos, rsKey.begin() + nNext);
		rnPos = nNext+1;
	}
	else
	{
		rsSubKey.assign(rsKey.begin() + rnPos, rsKey.end());
		rnPos = (size_t)-1;
	}

	return true;
}


template<typename WStrType, typename TData>
inline bool TreeCache<WStrType,TData>::GetTreeNode(const WStrType& rsFullKey, bool bReturnNearest, const TreeNode*& rpTreeNode) const
{
	if (rsFullKey.length() == 0)
	{
		rpTreeNode = &m_Root;
		return true;
	}

	FullKeyNodeMap::const_iterator cachedIt = m_FullKeyNodeMap.find(rsFullKey);

	if (cachedIt != m_FullKeyNodeMap.end())
	{
		rpTreeNode = cachedIt->second;
		return true;
	}
	
	if (!bReturnNearest)
		return false;

	rpTreeNode = &m_Root;

	size_t nPos = 0;
	WStrType sName;

	while(GetNextKey(rsFullKey, nPos, sName))
	{
		ChildTreeNodes::const_iterator it = rpTreeNode->Children.find(sName);

		if (it != rpTreeNode->Children.end())
			rpTreeNode = it->second;
		else
			return bReturnNearest;
	}

	return true;
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::GetCreateTreeNode(const WStrType& rsFullKey, TreeNode*& rpTreeNode)
{
	//Try some shortcuts
	if (rsFullKey.length() == 0)
	{
		rpTreeNode = &m_Root;
		return;
	}

	//Is it already in cache?
	FullKeyNodeMap::const_iterator cachedIt = m_FullKeyNodeMap.find(rsFullKey);

	if (cachedIt != m_FullKeyNodeMap.end())
	{
		rpTreeNode = cachedIt->second;
		return;
	}

	//Do full thing
	rpTreeNode = &m_Root;

	WStrType sName;
	size_t nPos = 0;

	while(GetNextKey(rsFullKey, nPos, sName))
	{
		ChildTreeNodes::iterator it = rpTreeNode->Children.find(sName);

		if (it == rpTreeNode->Children.end())
		{
			TreeNode* pChild = m_TreeNodePool.construct();
				
			rpTreeNode->Children[sName] = pChild;
			rpTreeNode = pChild;

			if (nPos != -1)
				m_FullKeyNodeMap[rsFullKey.substr(0, nPos-1)] = rpTreeNode;
			else
				m_FullKeyNodeMap[rsFullKey] = rpTreeNode;
		}
		else rpTreeNode = it->second;
	}
}



template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::GetFullKeyMap(FullKeyMap& rsFullKeyMap) const
{
	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	for(FullKeyNodeMap::const_iterator it = m_FullKeyNodeMap.begin(); it != m_FullKeyNodeMap.end(); ++it)
		if (it->second->bSet)
			rsFullKeyMap[it->first] = it->second->Data;
}


template<typename WStrType, typename TData>
inline bool TreeCache<WStrType,TData>::GetChildMap(WStrType sKey, SubKeyMap& rSubKeyMap, bool bIncludeEmpty) const
{
	MakePathHashSafe(sKey);

	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	const TreeNode* pTreeNode;

	if (!GetTreeNode(sKey, false, pTreeNode))
		return false;
	
	for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
		if (bIncludeEmpty || it->second->bSet)
			rSubKeyMap[it->first] = it->second->Data;

	return true;
}


template<typename WStrType, typename TData>
inline bool TreeCache<WStrType,TData>::GetTreeParent(const WStrType& rsFullKey, TreeNode*& rpParentTreeNode, TreeNode*& rpTreeNode, WStrType& rsName)
{
	if (rsFullKey.length() == 0)
		return false; //The root has no parent.

	if (!GetTreeNode(rsFullKey, false, (const TreeNode*&)rpTreeNode))
		return false;//This path wasn't in the tree anyway

	const size_t nSlash = rsFullKey.rfind('\\');

	if (nSlash != -1)
	{
		const bool bValid = GetTreeNode(rsFullKey.substr(0, nSlash), false, (const TreeNode*&)rpParentTreeNode);

		assert(bValid);

		rsName = rsFullKey.substr(nSlash+1);
	}
	else
	{
		rsName = rsFullKey;
		rpParentTreeNode = &m_Root; //No parent means it must be root
	}

	return true;
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::RemoveAndPrune(WStrType sFullKey, bool bRemoveAnyChildren)
{
	MakePathHashSafe(sFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pTreeNode;

	if (!GetTreeNode(sFullKey, false, (const TreeNode*&)pTreeNode))
		return;

	//Ensure at least set to empty.
	pTreeNode->bSet = false;
	pTreeNode->Data = TData();

	if ((!bRemoveAnyChildren && pTreeNode->Children.size() != 0) || pTreeNode == &m_Root)
		return;

	//So there aren't any children, or we are to remove them if there are any.

	WStrType sName;

	do
	{
		RemoveTreeNodeFromCache(sFullKey, pTreeNode, true); //Removes entry and all it's children

		//Now remove from parent, and keep pruning unset nodes down the tree to the root.

		const size_t nSlash = sFullKey.rfind(L'\\');

		if (nSlash != -1)
		{
			sName = sFullKey.substr(nSlash+1);
			sFullKey.resize(nSlash);

			GetTreeNode(sFullKey, false, (const TreeNode*&)pTreeNode);
		}
		else 
		{
			if (sFullKey.length() == 0)
				return; //Done

			sName = sFullKey;
			pTreeNode = &m_Root;
		}

		assert(pTreeNode != NULL);

		pTreeNode->Children.erase(sName);

		if (pTreeNode->Children.size() != 0)
			return; //Prune complete
	}
	while(pTreeNode != &m_Root && !pTreeNode->bSet);
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::RemoveBranch(WStrType sFullKey)
{
	MakePathHashSafe(sFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (sFullKey.length() > 0)
	{
		TreeNode* pTreeNode = NULL;
		TreeNode* pParentTreeNode = NULL;
		WStrType sName;

		if (!GetTreeParent(sFullKey, pParentTreeNode, pTreeNode, sName))
			return;//This path wasn't in the tree anyway

		assert(pTreeNode != NULL && pParentTreeNode != NULL);

		RemoveTreeNodeFromCache(sFullKey, pTreeNode, true);
		pParentTreeNode->Children.erase(sName);
	}
	else RemoveChildren(sFullKey, &m_Root, true);
}



template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::MoveBranch(WStrType sOldFullKey, WStrType sNewFullKey)
{
	MakePathHashSafe(sOldFullKey);
	MakePathHashSafe(sNewFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pOldParentTreeNode;
	TreeNode* pOldTreeNode;

	WStrType sOldName;

	if (!GetTreeParent(sOldFullKey, pOldParentTreeNode, pOldTreeNode, sOldName))
		return; //This path wasn't in the tree anyway

	assert(pOldParentTreeNode != NULL);


	//Add into new position
	TreeNode* pNewTreeNode = NULL;

	GetCreateTreeNode(sNewFullKey, pNewTreeNode);

	assert(pNewTreeNode != NULL);

	pNewTreeNode->bSet		= pOldTreeNode->bSet;
	pNewTreeNode->Data		= pOldTreeNode->Data;
	pNewTreeNode->Children	= pOldTreeNode->Children;

	AddTreeToFullKeyCache(sNewFullKey, pNewTreeNode);

	//Remove from old position

	RemoveTreeNodeFromCache(sOldFullKey, pOldTreeNode, false);
	m_TreeNodePool.destroy(pOldTreeNode);
	pOldParentTreeNode->Children.erase(sOldName);
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::RemoveChildren(const WStrType& rsFullKey, TreeNode *pTreeNode, bool bDestroy)
{
	for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
		RemoveTreeNodeFromCache(rsFullKey + L"\\" += it->first, it->second, bDestroy);
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::RemoveTreeNodeFromCache(const WStrType& rsFullKey, TreeNode *pTreeNode, bool bDestroy)
{
	m_FullKeyNodeMap.erase(rsFullKey);

	RemoveChildren(rsFullKey, pTreeNode, bDestroy);

	if (bDestroy)
		m_TreeNodePool.destroy(pTreeNode);
}


template<typename WStrType, typename TData>
inline void TreeCache<WStrType,TData>::AddTreeToFullKeyCache(const WStrType& rsFullKey, TreeNode *pTreeNode)
{
	m_FullKeyNodeMap[rsFullKey] = pTreeNode;

	for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
		AddTreeToFullKeyCache(rsFullKey + L"\\" += it->first, it->second);
}

