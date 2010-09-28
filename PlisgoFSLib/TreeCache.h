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


template<typename TData>
class TreeCache
{
public:

	typedef boost::unordered_map<std::wstring, TData>	FullKeyMap;
	typedef std::map<std::wstring, TData>				SubKeyMap;

	bool GetData(std::wstring sFullKey, TData& rData, bool bReturnNearest = false) const;

	void SetData(std::wstring sFullKey, TData& rData);

	void RemoveAndPrune(std::wstring sFullKey, bool bRemoveAnyChildren = false); //Remove key value, then remove any entries in branch that have no value
	void RemoveBranch(std::wstring sFullKey);

	void MoveBranch(std::wstring sOldFullKey, std::wstring sNewFullKey);

	void GetFullKeyMap(FullKeyMap& rsFullKeyMap) const;

	bool GetChildMap(std::wstring sFullKey, SubKeyMap& rSubKeyMap, bool bIncludeEmpty = false) const;

private:
	struct TreeNode;

	bool GetTreeNode(const std::wstring& rsKey, bool bReturnNearest, const TreeNode*& rpTreeNode) const;
	void GetCreateTreeNode(const std::wstring& rsKey, TreeNode*& rpTreeNode);

	bool GetTreeParent(const std::wstring& rsKey, TreeNode*& rpParentTreeNode, TreeNode*& rpTreeNode, std::wstring& rsName);

	bool GetNextKey(const std::wstring& rsKey, size_t& rnPos, std::wstring& rsSubKey) const;

	void RemoveChildren(const std::wstring& rsFullKey, TreeNode *pTreeNode, bool bDestroy);
	void RemoveTreeNodeFromCache(const std::wstring& rsFullKey, TreeNode *pTreeNode, bool bDestroy);

	void AddTreeToFullKeyCache(const std::wstring& rsFullKey, TreeNode *pTreeNode);


	typedef std::map<std::wstring, TreeNode*> ChildTreeNodes;

	struct TreeNode
	{
		TreeNode() { bSet = false; }

		bool			bSet;
		TData			Data;
		ChildTreeNodes	Children;
	};


	typedef boost::unordered_map<std::wstring, TreeNode*>	FullKeyNodeMap;
	typedef boost::object_pool<TreeNode>					TreeNodePool;

	mutable boost::shared_mutex		m_Mutex;
	TreeNode						m_Root;
	TreeNodePool					m_TreeNodePool;
	FullKeyNodeMap					m_FullKeyNodeMap;
};





template<typename TData>
inline bool TreeCache<TData>::GetData(std::wstring sFullKey, TData& rData, bool bReturnNearest) const
{
	MakePathHashSafe(sFullKey);

	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	const TreeNode* pTreeNode = NULL;

	if (!GetTreeNode(sFullKey, bReturnNearest, pTreeNode))
		return false;
	
	rData = pTreeNode->Data;

	return true;
}


template<typename TData>
inline void TreeCache<TData>::SetData(std::wstring sFullKey, TData& rData)
{
	MakePathHashSafe(sFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pTreeNode = NULL;

	GetCreateTreeNode(sFullKey, pTreeNode);

	assert(pTreeNode != NULL); //There is no fail!
	
	pTreeNode->Data = rData;
	pTreeNode->bSet = true;
}


template<typename TData>
bool TreeCache<TData>::GetNextKey(const std::wstring& rsKey, size_t& rnPos, std::wstring& rsSubKey) const
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


template<typename TData>
inline bool TreeCache<TData>::GetTreeNode(const std::wstring& rsFullKey, bool bReturnNearest, const TreeNode*& rpTreeNode) const
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
	std::wstring sName;

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


template<typename TData>
inline void TreeCache<TData>::GetCreateTreeNode(const std::wstring& rsFullKey, TreeNode*& rpTreeNode)
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

	std::wstring sName;
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



template<typename TData>
inline void TreeCache<TData>::GetFullKeyMap(FullKeyMap& rsFullKeyMap) const
{
	boost::shared_lock<boost::shared_mutex> readLock(m_Mutex);

	for(FullKeyNodeMap::const_iterator it = m_FullKeyNodeMap.begin(); it != m_FullKeyNodeMap.end(); ++it)
		if (it->second->bSet)
			rsFullKeyMap[it->first] = it->second->Data;
}


template<typename TData>
inline bool TreeCache<TData>::GetChildMap(std::wstring sKey, SubKeyMap& rSubKeyMap, bool bIncludeEmpty) const
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


template<typename TData>
inline bool TreeCache<TData>::GetTreeParent(const std::wstring& rsFullKey, TreeNode*& rpParentTreeNode, TreeNode*& rpTreeNode, std::wstring& rsName)
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


template<typename TData>
inline void TreeCache<TData>::RemoveAndPrune(std::wstring sFullKey, bool bRemoveAnyChildren)
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

	std::wstring sName;

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


template<typename TData>
inline void TreeCache<TData>::RemoveBranch(std::wstring sFullKey)
{
	MakePathHashSafe(sFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	if (sFullKey.length() > 0)
	{
		TreeNode* pTreeNode = NULL;
		TreeNode* pParentTreeNode = NULL;
		std::wstring sName;

		if (!GetTreeParent(sFullKey, pParentTreeNode, pTreeNode, sName))
			return;//This path wasn't in the tree anyway

		assert(pTreeNode != NULL && pParentTreeNode != NULL);

		RemoveTreeNodeFromCache(sFullKey, pTreeNode, true);
		pParentTreeNode->Children.erase(sName);
	}
	else RemoveChildren(sFullKey, &m_Root, true);
}



template<typename TData>
inline void TreeCache<TData>::MoveBranch(std::wstring sOldFullKey, std::wstring sNewFullKey)
{
	MakePathHashSafe(sOldFullKey);
	MakePathHashSafe(sNewFullKey);

	boost::unique_lock<boost::shared_mutex> lock(m_Mutex);

	TreeNode* pOldParentTreeNode;
	TreeNode* pOldTreeNode;

	std::wstring sOldName;

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


template<typename TData>
inline void TreeCache<TData>::RemoveChildren(const std::wstring& rsFullKey, TreeNode *pTreeNode, bool bDestroy)
{
	for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
		RemoveTreeNodeFromCache(rsFullKey + L"\\" += it->first, it->second, bDestroy);
}


template<typename TData>
inline void TreeCache<TData>::RemoveTreeNodeFromCache(const std::wstring& rsFullKey, TreeNode *pTreeNode, bool bDestroy)
{
	m_FullKeyNodeMap.erase(rsFullKey);

	RemoveChildren(rsFullKey, pTreeNode, bDestroy);

	if (bDestroy)
		m_TreeNodePool.destroy(pTreeNode);
}


template<typename TData>
inline void TreeCache<TData>::AddTreeToFullKeyCache(const std::wstring& rsFullKey, TreeNode *pTreeNode)
{
	m_FullKeyNodeMap[rsFullKey] = pTreeNode;

	for(ChildTreeNodes::const_iterator it = pTreeNode->Children.begin(); it != pTreeNode->Children.end(); ++it)
		AddTreeToFullKeyCache(rsFullKey + L"\\" += it->first, it->second);
}

