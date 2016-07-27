#include "StdAfx.h"
#include "HDPDish.h"
#include <math.h>

CHDPDish::CHDPDish(int wordNum) : m_wordNum(wordNum) , m_N_w(wordNum,0) , m_N(0) , m_numTables(0)
{
}

CHDPDish::~CHDPDish(void)
{
}


void CHDPDish::AddData( int w )
{
	m_N ++;
	m_N_w[w]++;
}

void CHDPDish::AddData( std::vector<int> &w )
{
	int num = (int)w.size();

	if( num )
	{
		int *pW = &w[0];
		for(int i=0 ; i<num ; i++ )
		{
			AddData( pW[i] );
		}
	}
}

void CHDPDish::DeleteData( int w )
{
	m_N--;
	m_N_w[w]--;

	if( m_N < 0 )
	{
		printf("Error : 料理を食べている人が負になりました\n");
	}

	if( m_N_w[w] < 0 )
	{
		printf("Error : 料理が割り当てられている単語%dが負になりました。\n" , w );
	}
}

void CHDPDish::DeleteData( std::vector<int> &w )
{
	int num = (int)w.size();

	if( num )
	{
		int *pW = &w[0];
		for(int i=0 ; i<num ; i++ )
		{
			DeleteData( pW[i] );
		}
	}
}



void CHDPDish::UpPopularity()
{
	m_numTables++;
}

void CHDPDish::DownPopularity()
{
	m_numTables--;

	if( m_numTables < 0 )
	{
		printf("Error : テーブルの数が負になりました。");
	}
}

int CHDPDish::GetPopularity()
{
	return m_numTables;
}

double CHDPDish::CalcLogLikilihood( int w )
{
	return log(HDP_ALPHA + m_N_w[w]) - log(m_wordNum*HDP_ALPHA + m_N);
}

double CHDPDish::CalcLogLikilihood( std::vector<int> w )
{
	int num = (int)w.size();
	double lik = 0;
	for(int i=0 ; i<num ; i++ )
	{
		lik += CalcLogLikilihood( w[i] );
	}
	return lik;
}