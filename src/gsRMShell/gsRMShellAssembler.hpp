#pragma once
/** @file gsRMShellAssembler.hpp

	@brief Provides assembler implementation for the RM shell equation.

	This file is part of the G+Smo library.

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Author(s): Y. Xia, HS. Wang

	Date:   2020-12-23
*/

#include <gsAssembler/gsVisitorPoisson.h> // Stiffness volume integrals
#include <gsAssembler/gsVisitorNeumann.h> // Neumann boundary integrals
#include <gsAssembler/gsVisitorNitsche.h> // Nitsche boundary integrals
#include <gsAssembler/gsVisitorDg.h>      // DG interface integrals
#include "gsRMShellAssembler.h"
#include "gsHyperMeshOut.h"

namespace gismo
{
	// >>> --------------------------------------------------------------------
	template<class T>
	void gsRMShellAssembler<T>::refresh()
	{
		// We use predefined helper which initializes the system matrix
		// rows and columns using the same test and trial space
		Base::scalarProblemGalerkinRefresh();
		// Base::setFixedDofs(m_coefMatrix); 
	}

	template<class T>
	void gsRMShellAssembler<T>::assemble()
	{
		GISMO_ASSERT(m_system.initialized(),
			"Sparse system is not initialized, call initialize() or refresh()");

		// Reserve sparse system
		m_system.reserve(m_bases[0], m_options, this->pde().numRhs());

		/*1. ��������غɵ�ʱ�򣬼�������Ҫ�õ��ſɱȾ���Ϊ�˱����ظ����㣬
		�ڼ��㵥Ԫ�ն����ʱ���Ȱ��ſɱȾ������������ŵ�rhs�У������ͳһ���غ�ֵ*/
		index_t m_cols = 1;
		if (m_BoundaryCondition.m_pPressure.numLoads() != 0)
		{
			// �����غɿ���û�У�Ҳ���ܲ�ֹһ��
			m_cols = m_BoundaryCondition.m_pPressure.numLoads();
		}

		// 2. ��Ϊû���ҵ�m_system�е����ɶ�����ô����ģ�������Ҫ������ʼ��m_system��ά��
		index_t alldof = m_dofpn * m_basis.size();
		m_system.matrix().resize(alldof, alldof);
		m_system.rhs().resize(alldof, m_cols);
		m_system.setZero();

		// 3. >>> �ն��� K �ĺ��ļ�����̣���Ƭ�� push �б�����
		// Assemble over all elements of the domain and applies
		gsRMShellVisitor<T> visitor(m_patches, m_dofmapper, m_material,
			m_BoundaryCondition.m_pPressure, m_SSdata, m_dofpn, m_inte);
		Base::template push<gsRMShellVisitor<T>>(visitor);
		// �����һ�䱨��Ļ����������for���
		/*for (index_t j = 0; j < m_patches.nPatches(); ++j)
		{
			this->apply(visitor, j);
		}*/

		// 4. ���ñ߽�����(˳�򲻿ɵߵ�)
		/*m_BoundaryCondition.setPressure(m_system);
		m_BoundaryCondition.setpLoad(m_system);
		m_BoundaryCondition.setDisp_constra(m_system);*/

		// Assembly is done, compress the matrix
		Base::finalize();

		// ����ȼ��θն���
		/*
		ofstream igak("../../TestResult/RM_Shell_IGAk.txt");
		for (index_t i=0; i<m_system.matrix().rows(); ++i)
		{
			for (index_t j = 0; j < m_system.matrix().cols(); ++j)
			{
				igak << setw(16) << m_system.matrix().coeffRef(i, j);
			}
			igak << endl;
		}
		igak.close();*/
	}

	// >>> --------------------------------------------------------------------
	// ��λ�ƽ�ת���ɾ�����ʽ [���Ƶ��� x ���ɶ�]
	template<class T>
	void gsRMShellAssembler<T>::constructSolution(gsMatrix<T>& solVector,
		gsMultiPatch<T>& result)
	{

		// The final solution is the deformed shell, therefore we add the
		// solVector to the undeformed coefficients
		result = m_patches;

		const index_t dim = m_basis.dim() + 1;

		for (index_t p = 0; p < m_patches.nPatches(); ++p)
		{
			// Reconstruct solution coefficients on patch p
			const index_t sz = m_basis[p].size();
			gsMatrix<T>& coeffs = result.patch(p).coefs();

			for (index_t i = 0; i < sz; ++i)
			{
				for (index_t j = 0; j < 3; ++j)
				{
					coeffs(i, j) = solVector(i * m_dofpn + j);
				}
			}
		}
	}

	// >>> --------------------------------------------------------------------
	// ����Ӧ��Ӧ��
	template<class T>
	void gsRMShellAssembler<T>::StressStrain(const gsMatrix<T>& solMatrix,
		gsMatrix<T>& StrainMat, gsMatrix<T>& StressMat)
	{
		// ��¼�ڵ��ظ��������ڵ�Ӧ��Ӧ����ظ������������ʵ�ʵ�Ӧ��Ӧ��
		gsMatrix<index_t> nodeRepeats;
		nodeRepeats.setZero(m_nurbsinfo.nurbs_sumcp, 1);

		index_t sumele = 0;        // ȫ�ֵ�Ԫ����
		index_t sumcps = 0;        // ȫ�ֿ��Ƶ����

		// SSdata ����Ƭ push_bach ��vector�еģ�����ֻ�ܷ�Ƭ����
		for (index_t np = 0; np < m_nurbsinfo.nurbs_sumpatch; ++np)
		{
			// ��Ԫ���Ƶ��������ǲ�ͬƬ�ϵĻ������״ο��ܲ�ͬ��
			index_t eleNodes = (m_nurbsinfo.nurbsdata[np].knot_vector[0].knotvec_degree + 1)
				* (m_nurbsinfo.nurbsdata[np].knot_vector[1].knotvec_degree + 1); // 9
			// ��Ԫ�Ļ��ֵ���������Ƭ�϶�����һ���ģ�
			index_t sumqus = m_SSdata.m_Ng[0].rows();  // 4

			// ��Ԫ��Ӧ���Ƶ��λ��
			gsVector<real_t> EleNodeDisp;
			EleNodeDisp.setZero(eleNodes * m_dofpn); // [9*6 x 1]

			// ��¼���ǻ��ֵ��Ӧ�䣬��ΪB���ڻ��ֵ�����
			gsMatrix<real_t> m_strain;
			gsMatrix<real_t> m_stress;
			m_strain.setZero(sumqus, m_dofpn); // [4 x 6]
			m_stress.setZero(sumqus, m_dofpn);

			// ��Ԫ��Ӧ�Ŀ��Ƶ��ŵ�ȫ�ֱ��
			gsMatrix<index_t> globalpNo;
			globalpNo.setZero(eleNodes, 1); // [9 x 1]

			// ��Ԫ����Ӧ��Ӧ��
			for (index_t i = 0; i < m_nurbsinfo.nurbsdata[np].patch_sumele; ++i)
			{
				// ��ȡ��Ԫ��Ӧ�� ��ǰƬ�Ŀ��Ƶ��� ��ȫ�ֱ��
				m_dofmapper.localToGlobal(m_SSdata.m_Pg[sumele + i], np, globalpNo);

				// ��Ԫ�ڵ��Ӧ��λ������
				for (index_t j = 0; j < eleNodes; ++j)
				{
					// [9*6 x 1]
					EleNodeDisp.segment(j * m_dofpn, m_dofpn) =
						solMatrix.block(globalpNo(j, 0) * m_dofpn, 0, m_dofpn, 1);
				}

				// ���ֵ��ϵ�Ӧ��Ӧ��
				for (index_t k = 0; k < sumqus; ++k)
				{
					// [4 x 6] = [4*6 x 9*6] x [9*6 x 1]
					m_strain.row(k) = (m_SSdata.m_Bg[sumele + i].block(k * m_dofpn, 0, m_dofpn, m_dofpn * eleNodes)
						* EleNodeDisp).transpose();
					// [4 x 6] = [4*6 x 6] x [4 x 6]
					m_stress.row(k) = m_SSdata.m_Dg[sumele + i].block(k * m_dofpn, 0, m_dofpn, m_dofpn)
						* m_strain.row(k).transpose();
				}

				// ���ֵ�Ļ�����ֵ
				gsMatrix<T> NGauss = m_SSdata.m_Ng[sumele + i];
				Eigen::MatrixXf MatrixXf_mat;
				Eigen::MatrixXf MatrixXf_bstress, MatrixXf_xstress;
				Eigen::MatrixXf MatrixXf_bstrain, MatrixXf_xstrain;
				gsMatrix2Matrixxf(NGauss, MatrixXf_mat);
				gsMatrix2Matrixxf(m_stress, MatrixXf_bstress);
				gsMatrix2Matrixxf(m_strain, MatrixXf_bstrain);

				// ����Ƶ��Ӧ��Ӧ��
				// [9 x 6]
				MatrixXf_xstress = MatrixXf_mat.colPivHouseholderQr().solve(MatrixXf_bstress);
				MatrixXf_xstrain = MatrixXf_mat.colPivHouseholderQr().solve(MatrixXf_bstrain);

				for (index_t m = 0; m < eleNodes; ++m)
				{
					for (index_t n = 0; n < m_dofpn; ++n)
					{
						StressMat(globalpNo(m, 0), n) += MatrixXf_xstress(m, n);
						StrainMat(globalpNo(m, 0), n) += MatrixXf_xstrain(m, n);
					}
					nodeRepeats(globalpNo(m, 0), 0)++;
				}
			}
			sumele += m_nurbsinfo.nurbsdata[np].patch_sumele;       // ��Ԫ����
			sumcps += m_nurbsinfo.nurbsdata[np].patch_sumcp;        // ���Ƶ�����
		}

		// �����ظ�����ĵ��ϵ�Ӧ��Ӧ��
		for (index_t i = 0; i < sumcps; ++i)
		{
			StressMat.row(i) /= nodeRepeats(i, 0);
			StrainMat.row(i) /= nodeRepeats(i, 0);
		}
	}

	// >>> --------------------------------------------------------------------
	// ��ͬ������ʽ�Ļ���ת��
	template<class T>
	void gsRMShellAssembler<T>::vector2Matrix(const gsMatrix<T>& solVector,
		gsMatrix<T>& result)
	{
		index_t sz = m_nurbsinfo.nurbs_sumcp;
		for (index_t i = 0; i < sz; ++i)
		{
			for (index_t j = 0; j < m_dofpn; ++j)
			{
				index_t tempr = i * m_dofpn + j;
				result(i, j) = solVector(tempr, 0);
			}
		}
	}

	template<class T>
	void gsRMShellAssembler<T>::VectorgoMatrix(const gsVector<T>& in_vector,
		gsMatrix<T>& result)
	{
		index_t size = in_vector.rows() / m_dofpn;
		for (index_t i = 0; i < size; ++i)
		{
			for (index_t j = 0; j < m_dofpn; ++j)
			{
				index_t tempr = i * m_dofpn + j;
				result(i, j) = in_vector(tempr, 0);
			}
		}
	}

	template <class T>
	void gsRMShellAssembler<T>::gsMatrix2Matrixxf(const gsMatrix<T>& a,
		Eigen::MatrixXf& aCopy)
	{
		int row = a.rows();
		int col = a.cols();

		aCopy.resize(row, col);
		for (int i = 0; i < row; ++i)
		{
			for (int j = 0; j < col; ++j)
			{
				aCopy(i, j) = a(i, j);
			}
		}
	}

	// ���ն����gsSparseMatrixת��ΪEigen::SparseMatrix,umf�����Ҫ��
	template <class T>
	void gsRMShellAssembler<T>::gsSparseToSparse(Eigen::SparseMatrix<T>& eiK)
	{
		// ��ѭ��
		for (index_t i = 0; i<m_system.matrix().outerSize(); ++i)
		{
			typename gsSparseMatrix<T, RowMajor,index_t>::iterator it1(m_system.matrix(), i);
			for (; it1; ++it1)
			{
				eiK.coeffRef(i, it1.col()) = it1.value();
			}
		}
		
	}

	template <class T>
	void gsRMShellAssembler<T>::gsMatrixtoVector(vector<T>& rhs_v)
	{
		// ��ѭ��
		for (index_t i = 0; i < m_system.rhs().rows(); ++i)
		{
			rhs_v.push_back(m_system.rhs()(i,0));
		}

	}
	
	// >>> --------------------------------------------------------------------=
	template <class T>
	void patch_test(gsRMShellAssembler<T>& ass)
	{
		//const gsDofMapper& mapper = ass.m_dofmapper.front();
		//dof sequences of all patches
		//vector<index_t> dof = mapper.m_dofs;
		
		//displacements
		gsMatrix<T> dis(ass.m_sum_nodes* ass.m_dofpn, 1);
		//int no_in_patch = dof.size() / ass.m_patches.nPatches();
		for (unsigned int i = 0; i < ass.m_patches.nPatches(); i++)
		{
			gsMatrix<T> coor = (ass.m_patches).patch(i).coefs();
			for (int j = 0; j < coor.rows(); j++)
			{
				//node sequence
				//int nod = dof[i * no_in_patch + j];
				int nod = ass.m_dofmapper.index(j,0,i);
				// disp in x direction u=0.002x
				dis(nod * 6, 0) = coor(j, 0) * 0.002;
				// disp in y direction v=-0.0006y
				dis(nod * 6 + 1, 0) = coor(j, 1) * (-0.0006);

				// shear model
				//dis(nod * 6, 0) = coor(j, 1) * 0.005;
				//dis(nod * 6 + 1, 0) = coor(j, 0) * 0.005;

				dis(nod * 6 + 2, 0) = 0;
				dis(nod * 6 + 3, 0) = 0;
				dis(nod * 6 + 4, 0) = 0;
				dis(nod * 6 + 5, 0) = 0;
			}
		}
		//patch test A stiffness matrix * displacements = F
		gsMatrix<T> res = ass.matrix() * dis;
		
		ofstream out;
		out.open("../../TestResult/RM_Shell_patchtest_F.txt");
		//gsOutputSparseMatrix(ass.m_matrix, "stiff.txt");
		for (int i = 0; i < ass.m_sum_nodes; ++i)
		{
			out << setw(3) << i << setw(20) << res(6 * i, 0);
			out << setw(20) << res(6 * i + 1, 0) << "\n";
		}
		double res_xsum = 0, res_ysum = 0;
		for (int i = 0; i < ass.m_sum_nodes; ++i)
		{
			res_xsum += res(i * 6, 0);
			res_ysum += res(i * 6 + 1, 0);
		}
		out << "Sum: \t" << res_xsum << "\t" << res_ysum << endl;
		out.close();

		// calculate the strains
		gsMatrix<> strainMat, stressMat;
		strainMat.setZero(ass.m_sum_nodes, ass.m_dofpn);
		stressMat.setZero(ass.m_sum_nodes, ass.m_dofpn);
		ass.StressStrain(dis, strainMat, stressMat);

		gsMatrix<> solMat; // [n x 6]
		solMat.setZero(ass.m_sum_nodes, ass.m_dofpn);
		ass.vector2Matrix(dis, solMat);

		gsOutputHMRes("../../TestResult/RM_Shell_patchtest.ascii",
			solMat, stressMat, strainMat);

		ofstream outf("../../TestResult/RM_Shell_patchtest_F.ascii");
		outf << "ALTAIR ASCII FILE\n"
			<< "$TITLE	 = Patch test\n"
			<< "$SUBCASE	 = 1	Subcase	1\n"
			<< "$binding	 = NODE\n"
			<< "$COLUMN_INFO	 = ENTITY_ID\n"
			<< "$RESULT_TYPE	 = Force(v)\n";
		{
			outf << "$TIME = " << setw(16) << 1 << "sec\n";
			for (int j = 0; j < ass.m_sum_nodes; ++j)
			{
				outf << setw(8) << j + 1;
				//���λ��
				{
					outf << setw(16) << res(j * 6, 0);
					outf << setw(16) << res(j * 6 + 1, 0);
					outf << setw(16) << 0 << endl;
				}
			}
		}
		outf.close();
	}

	
	// -------------------------------------------------------------------- <<<
}// namespace gismo
