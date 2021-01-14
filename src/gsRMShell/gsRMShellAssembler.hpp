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
#include "gsRMShellVisitor.hpp"
#include"gsMyBase/gsMyBase.h"
namespace gismo
{

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

        /*��������غɵ�ʱ�򣬼�������Ҫ�õ��ſɱȾ���Ϊ�˱����ظ����㣬
        �ڼ��㵥Ԫ�ն����ʱ���Ȱ��ſɱȾ������������ŵ�rhs�У������ͳһ���غ�ֵ*/
        index_t m_cols = 1;
        if (m_BoundaryCondition.m_pPressure.numLoads()!=0)
        { // �����غɿ���û�У�Ҳ���ܲ�ֹһ��
            m_cols = m_BoundaryCondition.m_pPressure.numLoads();
        }
        // ��Ϊû���ҵ�m_system�е����ɶ�����ô����ģ�������Ҫ������ʼ��m_system��ά��
        m_system.matrix().resize(m_dof_per_node * m_basis.size(), m_dof_per_node * m_basis.size());
        m_system.rhs().resize(m_dof_per_node * m_basis.size(), m_cols);
        // Clean the sparse system
        m_system.setZero(); //<< this call leads to a quite significant performance degrade!

        // Assemble over all elements of the domain and applies
        gsRMShellVisitor<T> visitor(m_patches,material, 
            m_BoundaryCondition.m_pPressure, m_dof_per_node, m_inte);
        Base::template push<gsRMShellVisitor<T>>(visitor);
       
        // ���ñ߽�����(˳�򲻿ɵߵ�)
        m_BoundaryCondition.setPressure(m_system);
        m_BoundaryCondition.setpLoad(m_system);
        m_BoundaryCondition.setDisp_constra(m_system);

        // Assembly is done, compress the matrix
        Base::finalize();
    }

    template<class T>
    void gsRMShellAssembler<T>::constructSolution(gsMatrix<T>& solVector,
        gsMultiPatch<T>& result) 
    {
        
		// The final solution is the deformed shell, therefore we add the
	    // solVector to the undeformed coefficients
		result = m_patches;

        const index_t dim = m_basis.dim()+1;

        for (size_t p = 0; p < m_patches.nPatches(); ++p)
        {
            // Reconstruct solution coefficients on patch p
            const size_t sz = m_bases[p].size();
            gsMatrix<T>& coeffs = result.patch(p).coefs();

            for (size_t i = 0; i < sz; ++i)
            {
                for (size_t j=0; j<3; ++j)
                {
                    coeffs(i, j) = solVector(i * m_dof_per_node + j);
                }
            }
        }
    }

}// namespace gismo
