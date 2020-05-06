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
import java.util.Scanner;

public class RobotsFrontend {

    private final static int BOARD_WIDTH = 45;
    private final static int BOARD_HEIGHT = 30;
    private final static int BOX_SIZE = 25;

    private static class MainView extends JPanel {

	private final Color background_color_ = new Color( 220, 220, 220 );
	private final Color background_color2_ = new Color( 200, 200, 200 );

	private final Color robot_color_ = new Color( 10, 10, 100 );
	private final Color human_color_ = new Color( 10, 100, 10 );
	private final Color  fire_color_ = new Color( 200, 10, 10 );

	String board = null;

	public String get_board() {
	    return board;
	}

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
		    final int y = (BOARD_HEIGHT-j - 1) * BOX_SIZE;
		    g2D.fillRect( x, y, BOX_SIZE, BOX_SIZE );
		    //System.out.println( x );
		    //System.out.println( y );
		}
		skip = !skip;
	    }

	    if( board != null ){
		int i = 0;
		int j = 0;
		if( board.length() != BOARD_HEIGHT * BOARD_WIDTH ){
		    System.out.println( board.length() );
		    System.out.println( BOARD_HEIGHT * BOARD_WIDTH );
		    System.exit( 1 );
		}
		for( int a = 0; a < board.length(); ++a ){
		    char c = board.charAt( a );
		    boolean draw = true;
		    switch( c ){
		    case( '1' )://ROBOT
			g2D.setColor( robot_color_ );
			break;
		    case( '2' )://HUMAN
			g2D.setColor( human_color_ );
			break;
		    case( '3' )://FIRE
			g2D.setColor( fire_color_ );
			break;
		    default:
			draw = false;
		    }
		    if( draw ){
			final int x = i * BOX_SIZE;
			final int y = (BOARD_HEIGHT-j - 1) * BOX_SIZE;
			g2D.fillOval( x, y, BOX_SIZE, BOX_SIZE );
		    }

		    ++j;
		    if( j == BOARD_HEIGHT ){
			j = 0;
			++i;
		    }
		}
	    }
	}

	public void paint( Graphics g ) {
	    Graphics2D g2D = (Graphics2D) g;
	    drawBackground( g2D );
	}

	public void updateboard( String newboard ){
	    board = newboard;
	    repaint();
	    revalidate();
	}
    }

    public static void main( String[] args ) {
	JFrame F = new JFrame( "Robots" );
	F.setDefaultCloseOperation( JFrame.EXIT_ON_CLOSE );
	//F.setPreferredSize(new Dimension( BOARD_WIDTH * BOX_SIZE, BOARD_HEIGHT * BOX_SIZE));
	F.setExtendedState( JFrame.MAXIMIZED_BOTH );
	// F.setUndecorated( true );
	MainView mv = new MainView();
	F.add( mv );
	F.setVisible( true );

	String second_to_last = "";

	Scanner input = new Scanner(System.in);
	while( input.hasNext() ){
	    String next = input.nextLine();
	    if( next == "EXIT" ){
		System.out.println( "EXIT!" );
		System.out.println( "UPDATE " + second_to_last );
		System.out.println( "UPDATE " + mv.get_board() );
		System.exit( 0 );
	    } else if( next.startsWith("UPDATE") ){
		second_to_last = mv.get_board();
		mv.updateboard( next.split(" ")[1] );
	    } else {
		System.out.println( next );
	    }
	}
    }

}
