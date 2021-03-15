#pragma once
/** @file gsNURBSinfo.h

	@brief RM shell's NURBS information.

	This file is not part of the G+Smo library.

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Author(s): HS. Wang

	Date:   2020-12-30
*/

#include <gismo.h>
#include <cctype>
#include "gsGeometryEvaluator.hpp"

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

		// ���캯��1
		gsNURBSinfo(gsMultiPatch<T> const& m_patch,
			gsMultiBasis<T> const& m_basis) :multi_patch(m_patch)
		{
			// ����1 ͨ��m_basis.basis(0)���txt�ļ���Ȼ���ٶ��룬Ч�����Ǻܺã�Ч�ʼ���
			// �����
			std::ofstream out_info("NURBSinfo.txt");
			out_info << m_basis.basis(0);
			out_info.close();
			// �ٶ���
			getKnotVectorInfo();
		}

		// ���캯��2
		gsNURBSinfo(gsMultiPatch<T> const& m_patch,
			gsMultiBasis<T> const& m_basis,
			const gsDofMapper& dofmapr)
		{
			// ����2 ʹ���ڲ�������ȡNURBS�����Ϣ
			/* nurbsdata[patch_info[kontvec_info]]
			* ����Ƭ����Ϣ[ ��Ƭ����Ϣ[ �ڵ�������Ϣ]] */
			// 2.1 ��ʼ��
			knotvec_info.knotvec_initialize();			// �����ڵ�������Ϣ
			patch_info.igapatch_initialize();			// ��Ƭ��NURBS��Ϣ
			vector<IGApatch_Info>().swap(nurbsdata);	// ����Ƭ��NURBS��Ϣ
			nurbs_sumcp = 0;							// ���Ƶ�����
			nurbs_sumele = 0;							// ��Ԫ����
			nurbs_sumpatch = 0;							// Ƭ����

			// 2.2 ѭ������patch
			nurbs_sumpatch = m_patch.nPatches();		// Ƭ����
			for (index_t i = 0; i < nurbs_sumpatch; ++i)
			{
				// 2.3 ��Ƭ��NURBS��Ϣ
				patch_info.patch_dim = m_basis[i].dim();
				patch_info.patch_sumcp = m_basis.size(i);
				patch_info.patch_sumele = m_basis[i].numElements();
				// 2.4 �ܿ��Ƶ��� �ܵ�Ԫ��
				nurbs_sumcp += patch_info.patch_sumcp;
				nurbs_sumele += patch_info.patch_sumele;
				// 2.5 �����ڵ�������Ϣ
				for (index_t j = 0; j < patch_info.patch_dim; ++j)
				{
					knotvec_info.knotvec_ele = m_basis[i].component(j).numElements(); // �ڵ�������Ӧ��Ԫ�� n-p
					knotvec_info.knotvec_size = m_basis[i].component(j).size();		// �ڵ�������Ӧ���Ƶ��� n
					knotvec_info.knotvec_degree = m_basis[i].component(j).degree(0);	// �ڵ������״� p
					real_t knot_span = 1.0 / knotvec_info.knotvec_ele;
					knotvec_info.knotvec_span = knot_span;

					// 2.6 ���ɽڵ��������������ھ��Ƚڵ㣩
					// [0 0 
					for (index_t m = 0; m < knotvec_info.knotvec_degree; ++m)
					{
						knotvec_info.knotvector.push_back(0.0);
					}
					// 0 to 1
					for (index_t k = 0; k <= knotvec_info.knotvec_ele; ++k)
					{
						real_t knot_temp = real_t(k * knot_span);
						knotvec_info.knotvector.push_back(knot_temp);
					}
					//  1 1]
					for (index_t n = 0; n < knotvec_info.knotvec_degree; ++n)
					{
						knotvec_info.knotvector.push_back(1.0);
					}
					patch_info.knot_vector.push_back(knotvec_info);
				}

				// 2.7 ���Ƶ�����& ���
				patch_info.phys_coor.setZero(patch_info.patch_sumcp, 3);
				gsMatrix<T>  coeffs = m_patch.patch(i).coefs();
				for (index_t no = 0; no < patch_info.patch_sumcp; ++no)
				{
					index_t tempno = dofmapr.index(no, 0, i);
					patch_info.phys_coor.row(tempno) = coeffs.row(no);
					//patch_info.phys_coor.row(dofmapr.index(no, i)) = coeffs.row(no);
					patch_info.cp_no.push_back(tempno);
				}

				// 2.8 ��������
				index_t sum_para = (patch_info.knot_vector[0].knotvec_ele + 1)
					* (patch_info.knot_vector[1].knotvec_ele + 1);
				patch_info.para_coor.resize(sum_para, 2);
				sum_para = 0;
				for (index_t m = patch_info.knot_vector[1].knotvec_degree;
					m <= patch_info.knot_vector[1].knotvec_ele; ++m)
				{
					for (index_t n = patch_info.knot_vector[0].knotvec_degree;
						n <= patch_info.knot_vector[0].knotvec_ele; ++n)
					{
						patch_info.para_coor(sum_para, 0) = patch_info.knot_vector[0].knotvector[n];
						patch_info.para_coor(sum_para, 1) = patch_info.knot_vector[1].knotvector[m];
						++sum_para;
					}
				}

				// 2.9 Greville ����
				m_patch.basis(i).anchors_into(patch_info.anch_coor);

				nurbsdata.push_back(patch_info);
			}
		}

		// ��������
		~gsNURBSinfo()
		{}

		// ��ȡ�ڵ�������Ϣ�ĺ���ģ��
		void getKnotVectorInfo();

		// ���K�ļ�
		// ����1 ����Geometry��Ϣ
		void OutputKfile(const gsGeometry<T>& Geo, const std::string& name);
		// ����2 ����NURBS��Ϣ
		void ExportKfile(const std::string& name);

		// �ַ���ת��Ϊ���ֵĺ���ģ��
		T trans_str_to_any(std::string s_str);
		//void trans_str_to_int(std::string s_str,T & res);

	public:
		// ����1�����ݽṹ
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
				vector<real_t>().swap(knot_vector);	// ���vector
				vector<real_t>().swap(para_coor);
				vector<real_t>().swap(phys_coor);
				knot_direction = 0;				// �������� xi or eta
				knot_degree = 0;				// �ڵ������״�
				knot_size = 0;				// �ڵ�������
				knot_minSpan = 0.0;				// ��С�ڵ��
				knot_maxSpan = 0.0;				// ���ڵ��
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

		// ����2�����ݽṹ
		// �����ڵ�������Ϣ
		class KnotVec_Info
		{
		public:
			KnotVec_Info()
			{
				knotvec_initialize();
			}
			~KnotVec_Info()
			{}

			void knotvec_initialize()
			{
				knotvec_ele = 0;
				knotvec_size = 0;
				knotvec_degree = 0;
				knotvec_span = 0;
				vector<real_t>().swap(knotvector);
			}

		public:
			index_t knotvec_ele;	// ��Ԫ�� n-p
			index_t knotvec_size;	// ���Ƶ��� n
			index_t knotvec_degree;	// �������״� p
			real_t  knotvec_span;	// �������ڵ�࣬��������[0,1]�ϵȾ�ֲ�
			vector<real_t>	knotvector; // �ڵ����� [0 to 1]
		};// end of class KnotVec_Info

		// ��Ƭ��NURBS��Ϣ
		class IGApatch_Info
		{
		public:
			IGApatch_Info()
			{
				igapatch_initialize();
			}
			~IGApatch_Info() {}

			void igapatch_initialize()
			{
				vector<KnotVec_Info>().swap(knot_vector); // ���vector
				para_coor.setZero();
				phys_coor.setZero();
				anch_coor.setZero();
				patch_dim = 0;
				patch_sumcp = 0;
				patch_sumele = 0;
			}

		public:
			vector<KnotVec_Info> knot_vector;	//	��Ƭ����ά�ȵĽڵ�����
			gsMatrix<real_t> para_coor;	// ��������
			gsMatrix<real_t> phys_coor;	// ��������
			gsMatrix<real_t> anch_coor; // Greville ���
			vector<index_t>	 cp_no;		// ���Ƶ���
			index_t patch_dim;			// ��Ƭ��ά�ȣ�һ����2
			index_t	patch_sumcp;		// ��Ƭ�Ŀ��Ƶ����� n*m
			index_t patch_sumele;		// ��Ƭ�ĵ�Ԫ���� ��n-p��*��m-q��
		};// end of class IGApatch_Info

	public:
		// ����1
		std::string		basis_type;	// ������������
		index_t			nurbs_dim;	// ��������ά��
		index_t			nurbs_node;	// �������
		real_t			tran_number; // ������������ת���ľֲ�����
		IGAinfo			info_iga;	// �����ڵ���������Ϣ
		vector<IGAinfo> igainfo;	// �������еĽڵ�����������
		gsMultiPatch<T> multi_patch;

	public:
		// ����2
		KnotVec_Info		knotvec_info;	// �����ڵ�������Ϣ
		IGApatch_Info		patch_info;		// ��Ƭ��NURBS��Ϣ
		vector<IGApatch_Info> nurbsdata;	// ����Ƭ��NURBS��Ϣ
		index_t				nurbs_sumcp;	// ���Ƶ�����
		index_t				nurbs_sumele;	// ��Ԫ����
		index_t				nurbs_sumpatch;	// Ƭ����
	};
}
