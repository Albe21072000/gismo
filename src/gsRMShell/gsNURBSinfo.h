/** @file gsNURBSinfo.h

    @brief RM shell's NURBS information.

    This file is not part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): HS. Wang

    Date:   2020-12-30
*/
#pragma once

#include <gismo.h>
#include <cctype>

namespace gismo
{
	/** @brief
		NURBS information of Reissner-Mindlin shell .

		It sets up an assembler and assembles the system patch wise and combines
		the patch-local stiffness matrices into a global system by various methods
		(see gismo::gsInterfaceStrategy). It can also enforce Dirichlet boundary
		conditions in various ways (see gismo::gsDirichletStrategy).

		\ingroup Assembler
	*/
	template < class T>
	class gsNURBSinfo
	{

	public:
		// Ĭ�Ϲ��캯��
		gsNURBSinfo()
		{}
		//typedef memory::shared_ptr<gsNURBSinfo> Ptr;
		//typedef memory::unique_ptr<gsNURBSinfo> uPtr;

		// ���ƹ��캯��
		gsNURBSinfo(gsMultiPatch<T> const& m_patch,
			gsMultiBasis<T> const& m_basis)
		{
			// �����
			std::ofstream out_info("NURBSinfo.txt");
			out_info << m_basis.basis(0);
			out_info.close();
			// �ٶ���
			
			getKnotVectorInfo();
			
		}

		// ��������
		~gsNURBSinfo()
		{}

		// ��ȡ�ڵ�������Ϣ�ĺ���ģ��
		void getKnotVectorInfo();

		// �ַ���ת��Ϊ���ֵĺ���ģ��
		T trans_str_to_any(std::string s_str);
		//void trans_str_to_int(std::string s_str,T & res);
		
	public:
		class IGAinfo
		{
		public:
			IGAinfo() 
			{
				initialize();
			}
			~IGAinfo() {}

			void initialize()
			{
				vector<real_t>().swap(knot_vector);// ���vector
				vector<real_t>().swap(para_coor);
				vector<real_t>().swap(phys_coor);
				//knot_vector.swap(vector<real_t>());// ���vector //error!
				//para_coor.swap(vector<real_t>());
				//phys_coor.swap(vector<real_t>());
				knot_direction	= 0;
				knot_degree	= 0;
				knot_size	= 0;
				knot_minSpan	= 0.0;
				knot_maxSpan	= 0.0;
			}
		public:
			
			vector<real_t> knot_vector;	//	�ڵ�����
			vector<real_t> para_coor;	//	��������
			vector<real_t> phys_coor;	//	��������
			index_t	knot_direction;
			index_t	knot_degree;
			index_t	knot_size;
			real_t	knot_minSpan;
			real_t  knot_maxSpan;
		private:

		};

	public:
		std::string basis_type;	// ������������
		index_t nurbs_dim; // ��������ά��
		index_t	nurbs_node;	// �������
		real_t  tran_number; // ������������ת���ľֲ�����
		IGAinfo info_iga;	// �����ڵ���������Ϣ
		vector<IGAinfo> igainfo; // �������еĽڵ�����������
		//vector<string> basis_info;
		
		

	};
}
