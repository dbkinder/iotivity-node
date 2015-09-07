module.exports = function( grunt ) {

var path = require( "path" ),
	spawn = require( "child_process" ).spawn;

grunt.task.registerTask( "test", "Run the test suite", function() {
	var done = this.async();
	spawn( "node", [ path.join( process.cwd(), "tests", "suite.js" ) ], { stdio: "inherit" } )
		.on( "close", function( code ) {
			done( code === 0 || code === null );
		} );
} );

};
