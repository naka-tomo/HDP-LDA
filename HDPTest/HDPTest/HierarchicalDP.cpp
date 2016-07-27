#include "StdAfx.h"
#include "HierarchicalDP.h"
#include "utility.h"
#include "randlib/randlib.h"
#include <string.h>
#include <float.h>
#include <math.h>
#include <windows.h>
#include <algorithm>



CHierarchicalDP::CHierarchicalDP(void)
{
	m_documents_d = NULL;
	m_dataDim = 0;
	m_dataNum = 0;
	m_dataIndices_d = NULL;
	m_P = new double[BUFFER_SIZE];

	// ガンマ分布の期待値 x～Gamma(A,B) ⇒ E(x) = A/B
	m_lambda = HDP_CONCPARA_PRIOR_A / HDP_CONCPARA_PRIOR_B;
	m_gamma = HDP_CONCPARA_PRIOR_A / HDP_CONCPARA_PRIOR_B;
}

CHierarchicalDP::~CHierarchicalDP(void)
{
	delete [] m_P;
}

void CHierarchicalDP::Release()
{
	for(int d=0 ; d<m_dataNum ; d++ )
	{
		delete [] m_documents_d[d].wordID_l;
		delete [] m_documents_d[d].table_l;
	}
	
	delete [] m_documents_d;
	m_documents_d = NULL;

	delete [] m_dataIndices_d;
	m_dataIndices_d = NULL;
}

void CHierarchicalDP::SetData( double *data[] , int dataNum , int dataDim )
{
	Release();

	m_dataNum = dataNum;
	m_dataDim = dataDim;


	// メモリ確保
	m_documents_d = new Document[dataNum];
	m_dataIndices_d = new int[dataNum];


	for(int d=0 ; d<m_dataNum ; d++ )
	{
		int length = 0;

		// データのインデックスを格納
		m_dataIndices_d[d] = d;

		// 合計を計算
		for(int w=0 ; w<m_dataDim ; w++ ) length += (int)data[d][w];

		m_documents_d[d].length = length;
		m_documents_d[d].wordID_l = new int[length];
		m_documents_d[d].table_l = new int[length];
		m_documents_d[d].tables_t.push_back( CHDPTable() );	// 空のテーブル追加

		// 単語を代入
		for(int w=0, l=0 ; w<dataDim ; w++ )
		{
			for(int j=0 ; j<(int)data[d][w] ; j++ , l++ )
			{
				m_documents_d[d].wordID_l[l] = w;
				m_documents_d[d].table_l[l] = -1; 
			}
		}
	}

	// 一つ目の料理を作っておく
	m_dishes_k.clear();
	m_dishes_k.push_back( CHDPDish(dataDim) );
}


void CHierarchicalDP::Update()
{
	// データのインデックスをランダムで並び替え
	std::random_shuffle( m_dataIndices_d , m_dataIndices_d+m_dataNum , RandD );

	// テーブル決定
	for(int i=0 ; i<m_dataNum ; i++ )		// 各チェーン店でforループ
	{
		int d = m_dataIndices_d[i]; 
		int len = m_documents_d[d].length;	// 客の人数
		for(int l=0 ; l<len ; l++ )
		{
			UpdateTable( m_documents_d[d] , l );
		}
	}


	// 料理決定
	for(int i=0 ; i<m_dataNum ; i++ )		// 各チェーン店でforループ
	{
		int d = m_dataIndices_d[i];
		int T = (int)m_documents_d[d].tables_t.size()-1;	// 最後のテーブルは空のテーブルなので料理は置かない
		for(int t=0 ; t<T ; t++ )
		{
			UpdateDish( m_documents_d[d].tables_t[t] );
		}
	}

	// concentrate parameterのアップデート
	m_lambda = SamplingLambda( m_lambda );
	m_gamma = SamplingGamma( m_gamma );
}



void CHierarchicalDP::UpdateTable( Document &d , int l )
{
	int t = d.table_l[l];
	int w = d.wordID_l[l];
	//int k = d.dish_l[l];

	if( t!=-1 )
	{
		d.tables_t[t].DeleteData( w );		// データをテーブルから削除
		int k = d.tables_t[t].GetDishID();	// テーブルに置かれている料理を取得
		m_dishes_k[k].DeleteData( w );		// データを料理から削除
	}
	t = SamplingTable( d.tables_t , w );	// テーブル番号をサンプリング

	// サンプリングされたテーブルに座る
	d.table_l[l] = t;
	d.tables_t[t].AddData( w );	

	// 新しいテーブルに座った場合空のテーブルを追加
	if( t == d.tables_t.size()-1 )
	{
		UpdateDish( d.tables_t[t] );			// 新しいテーブルの料理を決定
		d.tables_t.push_back( CHDPTable() );	// 新しいテーブルのための空のテーブルを作る
	}
						
	int k = d.tables_t[t].GetDishID();				// 料理を食べる人に追加
	m_dishes_k[k].AddData( w );


	// 空になったテーブルを削除
	DeleteEmptyTables( d );
}

void CHierarchicalDP::DeleteEmptyTables( Document &d )
{
	int numTable = (int)d.tables_t.size() - 1;

	// 空のテーブルを探す
	for(int t=0 ; t<numTable ; t++ )
	{
		if( d.tables_t[t].GetDataNum() == 0 )
		{
			DeleteTable( d , t );
			break;
		}
	}
}

void CHierarchicalDP::DeleteTable( Document &d , int t )
{
	int T = (int)d.tables_t.size()-1;		// 最後のテーブルは、からのテーブルなので-1
	int k= d.tables_t[t].GetDishID();	// テーブルに乗っていた料理
	d.tables_t.erase( d.tables_t.begin() + t );

	// 料理の人気を下げる
	m_dishes_k[k].DownPopularity();

	// 消したテーブル分idをづらす
	for(int l=0 ; l<d.length ; l++ )
	{
		if( d.table_l[l] > t )
		{
			d.table_l[l]--;
		}
	}
}


int CHierarchicalDP::SamplingTable( std::vector<CHDPTable> &tables , int w )
{
	int numTables = (int)tables.size();
	int numDishes = (int)m_dishes_k.size();
	double *P = m_P;
	int newTable = -1;
	double max = -DBL_MAX;


	// 最後のテーブルは空のテーブル
	if( numTables == 1 ) return 0;

	// 各テーブルに属する対数尤度
	for(int t=0 ; t<numTables-1 ; t++ )
	{
		int k = tables[t].GetDishID();
		P[t] = m_dishes_k[k].CalcLogLikilihood( w );
	}
	P[numTables-1] = m_dishes_k[numDishes-1].CalcLogLikilihood( w );

	// 最大値を探す
	for(int t=0 ; t<numTables ; t++ ) if( max < P[t] ) max = P[t];


	// 値が小さくなりすぎるため、最大値で引く
	// 各テーブルの人気をかける
	// サンプリングのために累積確率にする
	P[0] = exp(P[0] - max) * tables[0].GetDataNum();
	for(int t=1 ; t<numTables-1 ; t++ ) P[t] = P[t-1] + exp(P[t] - max) * tables[t].GetDataNum(); 

	// 新たなテーブルを生成する確率
	P[numTables-1] = P[numTables-2] + exp(P[numTables-1] - max) * m_lambda;

	// サンプリングするための乱数を発生
	double rand = RandF() * P[numTables-1];

	// 計算した確率に従って新たなテーブルを選択
	for(newTable=0 ; newTable<numTables-1 ; newTable++ )
	{
		if( P[newTable] >= rand ) break;
	}
	return newTable;
}


void CHierarchicalDP::UpdateDish( CHDPTable &table )
{
	int k = table.GetDishID();
	std::vector<int> &w = table.GetDatas();

	if( k!=-1 )
	{
		RemoveDishFromTable( table , k );	// テーブルから料理を下げる
	}

	k = SamplingDish( w );					// テーブルに着いている人に新たな料理をサンプリング

	// 新しい料理が選択された場合は、
	// vectorの最後に空の料理（新メニュー候補）を追加
	if( k == m_dishes_k.size()-1 )
	{
		m_dishes_k.push_back( CHDPDish(m_dataDim) );
	}

	PutDishOnTable( table , k );	// テーブルに料理を置く
	DeleteEmptyDish();				// 誰も食べてない料理を削除
}


int CHierarchicalDP::SamplingDish( std::vector<int> &w )
{
	int numDishes = (int)m_dishes_k.size();
	double *P = m_P;
	int newDish = -1;
	double max = -DBL_MAX;

	// まだ料理は誰も食べていないので、新メニュー
	// indexの最後が新メニューを表している
	if( numDishes == 1 ) return 0;

	// 人wが料理kを好む対数尤度
	for(int k=0 ; k<numDishes ; k++ )
	{
		P[k] = m_dishes_k[k].CalcLogLikilihood( w );
	}

	// 最大値を探す
	for(int k=0 ; k<numDishes ; k++ ) if( max < P[k] ) max = P[k];


	P[0] = exp(P[0] - max) * m_dishes_k[0].GetPopularity();
	for(int k=1 ; k<numDishes-1 ; k++ ) P[k] = P[k-1] + exp(P[k] - max) * m_dishes_k[k].GetPopularity(); 

	// 新たなテーブルを生成する確率
	P[numDishes-1] = P[numDishes-2] + exp(P[numDishes-1] - max) * m_gamma;

	// サンプリングするための乱数を発生
	double rand = RandF() * P[numDishes-1];

	// 計算した確率に従って新たなテーブルを選択
	for(newDish=0 ; newDish<numDishes-1 ; newDish++ )
	{
		if( P[newDish] >= rand ) break;
	}

	return newDish;
}


void CHierarchicalDP::DeleteEmptyDish()
{
	int K = (int)m_dishes_k.size()-1;

	for(int k=0 ; k<K ; k++ )
	{
		if( m_dishes_k[k].GetPopularity() == 0 )
		{
			DeleteDish( k );
			break;
		}
	}
}

void CHierarchicalDP::DeleteDish(int k)
{
	m_dishes_k.erase( m_dishes_k.begin() + k );

	// 全データとテーブルの料理IDを更新
	for(int d=0 ; d<m_dataNum ; d++ )
	{
		int T = (int)m_documents_d[d].tables_t.size();
		CHDPTable *tables = &(m_documents_d[d].tables_t[0]);

		for(int t=0 ; t<T ; t++ )
		{
			tables[t].DeleteDish( k );	// メニュー変更
		}
	}
}


void CHierarchicalDP::PutDishOnTable( CHDPTable &table , int k )
{
	CHDPDish &dish = m_dishes_k[k];
	table.PutDish( k );						// 料理を置く
	dish.AddData( table.GetDatas() );		// 食べている人を追加
	dish.UpPopularity();					// 料理の人気UP
}

void CHierarchicalDP::RemoveDishFromTable( CHDPTable &table , int k )
{
	CHDPDish &dish = m_dishes_k[k];
	dish.DeleteData( table.GetDatas() );	// 食べている人を除外
	dish.DownPopularity();					// 料理の人気DOWN
}



std::vector<std::vector<double>> CHierarchicalDP::GetPz_dk()
{
	std::vector<std::vector<double>> lik_dk;
	lik_dk.resize( m_dataNum );
	for(int d=0 ; d<m_dataNum ; d++ ) lik_dk[d].resize( m_dishes_k.size() );

	for(int d=0 ; d<m_dataNum ; d++ )
	{
		double sum = 0;
		std::vector<CHDPTable> &tables = m_documents_d[d].tables_t;
		for(int t=0 ; t<tables.size()-1 ; t++ )
		{
			int k = tables[t].GetDishID();
			double lik = tables[t].GetDataNum();
			lik_dk[d][k] += lik;
			sum += lik;
		}

		// 正規化
		for(int k=0 ; k<lik_dk[d].size() ; k++ )
		{
			lik_dk[d][k] /= sum;
		}
	}

	return lik_dk;
}

std::vector<int> CHierarchicalDP::GetClassificationResult_d()
{
	std::vector<std::vector<double>> Pz_dk = GetPz_dk();
	std::vector<int> classRes;

	for(int d=0 ; d<Pz_dk.size() ; d++ )
	{
		double max = -DBL_MAX;
		int maxIdx = -1;
		for(int k=0 ; k<Pz_dk[d].size() ; k++ )
		{
			if( max < Pz_dk[d][k] )
			{
				max = Pz_dk[d][k];
				maxIdx = k;
			}
		}
		classRes.push_back( maxIdx );
	}

	return classRes;
}



void CHierarchicalDP::SaveResult( const char *dir )
{
	char dirname[256];
	char filename[256];

	strcpy( dirname , dir );
	int len = (int)strlen( dir );
	if( len==0 || dir[len-1] != '\\' || dir[len-1] != '/' ) strcat( dirname , "\\" );
	
	CreateDirectory( dirname , NULL );

	std::vector< std::vector<double> > Pz = GetPz_dk();
	sprintf( filename , "%sPz.txt" , dirname );
	SaveMatrix( Pz , (int)Pz[0].size() , (int)Pz.size() , filename );

	std::vector<int> classRes = GetClassificationResult_d();
	sprintf( filename , "%sClusteringResult.txt" , dirname );
	SaveArray( classRes , (int)classRes.size() , filename );	

}


double CHierarchicalDP::SamplingLambda( double oldLambda )
{
	for(int i=0 ; i<50 ; i++ )
	{
		float gammaB = 0;	// ガンマ関数スケールパラメータ
		float gammaA = 0;	// ガンマ関数形状パラメータ
		int numAllTables = 0;

		for(int d=0 ; d<m_dataNum ; d++ )
		{
			int len = m_documents_d[d].length;

			// ベータ分布からサンプル生成
			float w = genbet( (float)oldLambda+1 , (float)len );
			gammaB -= log(w);
			
			// 二値分布からサンプリング
			int s = (RandF() * (oldLambda + len)) < len ? 1 : 0;
			gammaA -= s;

			// テーブルの総数を計算
			numAllTables += (int)m_documents_d[d].tables_t.size()-1;
		}

		// 事後分布のパラメタを計算
		gammaA += (float)(HDP_CONCPARA_PRIOR_A + numAllTables);
		gammaB += (float)HDP_CONCPARA_PRIOR_B;

		// 更新
		oldLambda = (double)gengam( gammaB , gammaA );
	}

	return oldLambda;
}


double CHierarchicalDP::SamplingGamma( double oldGamma )
{
	// テーブルの総数を計算
	int numAllTables = 0;
	for(int k=0 ; k<m_dishes_k.size() ; k++ )
	{
		numAllTables += m_dishes_k[k].GetPopularity();
	}

	for(int i=0 ; i<20 ; i++ )
	{
		float gammaB = 0;	// ガンマ関数スケールパラメータ
		float gammaA = 0;	// ガンマ関数形状パラメータ
		int numDish = (int)m_dishes_k.size();

		// ベータ分布からサンプル生成
		float w = genbet( (float)oldGamma+1 , (float)numAllTables );

		// 二値の分布をサンプリング
		int s = (RandF() * (oldGamma + numDish)) < numDish ? 1 : 0;
		gammaA = (float)(HDP_CONCPARA_PRIOR_A + numDish - s);
		gammaB = (float)(HDP_CONCPARA_PRIOR_B - log(w));

		// 更新
		oldGamma = (double)gengam( gammaB , gammaA );
	}

	return oldGamma;
}


double RandF()
{
	unsigned int val;
	rand_s(&val);
	return (double)val/UINT_MAX;
}

unsigned int RandD(unsigned int max )
{
	unsigned int val;
	rand_s(&val);
	return val%max;
}