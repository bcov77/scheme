#include <gtest/gtest.h>

#include "scheme/actor/BackboneActor.hh"
#include "scheme/actor/ActorConcept_io.hh"
#include "scheme/objective/ObjectiveFunction.hh"
#include "scheme/objective/ObjectiveVisitor.hh"
#include "scheme/numeric/X1dim.hh"

#include "scheme/io/dump_pdb_atom.hh"

#include <boost/foreach.hpp>



#include <stdint.h>
#include <fstream>

#include <Eigen/Geometry>

namespace scheme { namespace actor { namespace test_bbactor {

using std::cout;
using std::endl;

typedef Eigen::Transform<double,3,Eigen::AffineCompact> Xform;
typedef Eigen::AngleAxis<double> AA;
typedef Eigen::Vector3d V3;

TEST( BackboneActor, test_from_N_CA_C ){
	// ATOM     32  N   ALA A  12      29.741  16.197  72.632  1.00 35.94           N
	// ATOM     33  CA  ALA A  12      30.303  15.929  71.289  1.00 43.98           C
	// ATOM     34  C   ALA A  12      29.139  15.623  70.352  1.00 44.26           C
	// ATOM     36  CB  ALA A  12      31.230  14.716  71.381  1.00 50.42           C
	V3 n ( 29.741 , 16.197 , 72.632 );
	V3 ca( 30.303 , 15.929 , 71.289 );
	V3 c ( 29.139 , 15.623 , 70.352 );
	BackboneActor<Xform> bb( n, ca, c, 'A', 'H' );
	cout << bb.position_.translation().transpose() << endl;

	// cout << "0 " << (bb.position_*V3(0,0,0)).transpose() << endl;
	// cout << "X " << (bb.position_*V3(3,0,0)).transpose() << endl;
	// cout << "Y " << (bb.position_*V3(0,3,0)).transpose() << endl;
	// cout << "Z " << (bb.position_*V3(0,0,3)).transpose() << endl;


	// cout << "n-ca: " << ( n - ca ).normalized().transpose() << endl;
	// cout << "c-ca: " << ( c - ca ).normalized().transpose() << endl;
	// cout << "Ux  : " << (bb.position_.rotation() * V3(1,0,0)).transpose() << endl;
	// cout << "Uy  : " << (bb.position_.rotation() * V3(0,1,0)).transpose() << endl;	
	// cout << "Uz  : " << (bb.position_.rotation() * V3(0,0,1)).transpose() << endl;		

	V3 n_ca = ( n - ca ).normalized().transpose();
	V3 ximg = bb.position_.rotation() * V3(1,0,0);

	ASSERT_LE( (n_ca-ximg).norm(), 0.000001 );

	ASSERT_LE( (bb.position_.translation() - ca).norm() - 2.48737, 0.0001 ); // len of centroid to ca
}


}
}
}
