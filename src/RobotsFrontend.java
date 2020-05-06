import javax.swing.JFrame;
import java.awt.Dimension;
import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.geom.AffineTransform;
import java.awt.geom.Line2D;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import javax.swing.JPanel;

public class RobotsFrontend {

    private final static int BOARD_WIDTH = 45;
    private final static int BOARD_HEIGHT = 30;
    private final static int BOX_SIZE = 25;

    private static class MainView extends JPanel {

	private final Color background_color_ = new Color( 220, 220, 220 );
	private final Color background_color2_ = new Color( 200, 200, 200 );

	public void drawBackground( Graphics2D g2D ) {
	    g2D.setColor( background_color_ );
	    g2D.fillRect( 0, 0, getWidth(), getHeight() );
	    g2D.setColor( background_color2_ );
	    boolean skip = false;
	    for( int i = 0; i < BOARD_WIDTH; ++i ){
		for( int j = 0; j < BOARD_HEIGHT; ++j ){
		    skip = !skip;
		    if( skip ) continue;

		    final int x = i * BOX_SIZE;
		    final int y = j * BOX_SIZE;
		    g2D.fillRect( x, y, BOX_SIZE, BOX_SIZE );
		    //System.out.println( x );
		    //System.out.println( y );
		}
		skip = !skip;
	    }
	}

	public void paint( Graphics g ) {
	    Graphics2D g2D = (Graphics2D) g;
	    drawBackground( g2D );
	}
    }

    public static void main( String[] args ) {
	JFrame F = new JFrame( "Robots" );
	F.setDefaultCloseOperation( JFrame.EXIT_ON_CLOSE );
	//F.setPreferredSize(new Dimension( BOARD_WIDTH * BOX_SIZE, BOARD_HEIGHT * BOX_SIZE));
	F.setExtendedState( JFrame.MAXIMIZED_BOTH );
	// F.setUndecorated( true );
	F.add( new MainView() );
	F.setVisible( true );
    }

}
