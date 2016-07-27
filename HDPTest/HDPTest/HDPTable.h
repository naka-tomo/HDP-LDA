#pragma once
/*******************************************************************

HDPTable.h		developed by naka_t	2011.01.12

	HDPのテーブルの情報を保持するクラス

  Copyright (C) 2011  naka_t <naka_t@apple.ee.uec.ac.jp>
*******************************************************************/
#include <vector>


class CHDPTable
{
public:
	CHDPTable(void);
	~CHDPTable(void);

	void AddData(int w );		// テーブルに人（データ）が座る
	void DeleteData( int w );	// テーブルから人（データ）を除く
	void PutDish( int k );		// 料理（トピック）をテーブルに置く
	void DeleteDish( int k );	// 全チェーン店から料理がなくなった場合にメニューを変更

	int GetDataNum(){ return (int)m_datasOnTable.size(); }			// テーブルに座っている人数を取得
	int GetDishID(){ return m_dishID; }							// テーブルに置かれている料理を取得
	std::vector<int> &GetDatas(){ return m_datasOnTable; }		// テーブルに座っている人（データ）を取得

protected:
	std::vector<int> m_datasOnTable;		// テーブルに置かれているデータ
	int m_dishID;							// 料理ID
};
