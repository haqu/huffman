/*
 *  Huffman coding algorithm
 *  by Sergey Tikhonov (st@haqu.net)
 * 
 *  Usage: huffman [OPTIONS] input [output]
 *    The default action is to encode input file.
 *    -d  Decode file.
 * 
 *  Examples:
 *    huffman input.txt
 *    huffman input.txt encoded.txt
 *    huffman -d encoded.txt
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#define NL "\r\n"
#else
#define NL "\n"
#endif

#include <string>
#include <vector>
#include <map>
#include <cassert>

using namespace std;

struct pnode
{
  char ch; // char
  float p; // probability
};

static int pnode_compare( const void *elem1, const void *elem2 )
{
  const pnode a = *(pnode*)elem1;
  const pnode b = *(pnode*)elem2;
  if( a.p < b.p ) return 1; // 1 - less (reverse for decreasing sort)
  else if( a.p > b.p ) return -1;
  return 0;
}

struct treenode : public pnode
{
  char lcode;
  char rcode;
  treenode *left; // left child
  treenode *right; // right child
};

class Coder
{
private:
  int tsize; // table size (number of chars)
  pnode *ptable; // table of probabilities
  map<char, string> codes; // codeword for each char

public:
  void Encode( const char *inputFilename, const char *outputFilename )
  {
    map<char, int> freqs; // frequency for each char from input text
    int i;

    //  Opening input file
    //
    FILE *inputFile;
    inputFile = fopen( inputFilename, "r" );
    assert( inputFile );

    //  Counting chars
    //
    char ch; // char
    unsigned total = 0;
    while( fscanf( inputFile, "%c", &ch ) != EOF )
    {
      freqs[ch]++;
      total++;
    }
    tsize = (int)freqs.size();

    //  Building decreasing freqs table
    //
    ptable = new pnode[tsize];
    assert( ptable );
    float ftot = float(total);
    map<char, int>::iterator fi;
    for( fi=freqs.begin(),i=0; fi!=freqs.end(); ++fi,++i )
    {
      ptable[i].ch = (*fi).first;
      ptable[i].p = float((*fi).second) / ftot;
    }
    qsort( ptable, tsize, sizeof(pnode), pnode_compare );

    //  Encoding
    //
    EncHuffman();

    //  Opening output file
    //
    FILE *outputFile;
    outputFile = fopen( outputFilename, "wb" );
    assert( outputFile );

    //  Outputing ptable and codes
    //
    printf( "%i"NL, tsize );
    fprintf( outputFile, "%i"NL, tsize );
    for( i=0; i<tsize; i++ )
    {
      printf("%c\t%f\t%s"NL, ptable[i].ch, ptable[i].p, codes[ptable[i].ch].c_str() );
      fprintf(outputFile, "%c\t%f\t%s"NL, ptable[i].ch, ptable[i].p, codes[ptable[i].ch].c_str() );
    }

    //  Outputing encoded text
    //
    fseek( inputFile, SEEK_SET, 0 );
    printf(NL);
    fprintf(outputFile, NL);
    while( fscanf( inputFile, "%c", &ch ) != EOF )
    {
      printf("%s", codes[ch].c_str());
      fprintf(outputFile, "%s", codes[ch].c_str());
    }
    printf(NL);

    //  Cleaning
    //
    codes.clear();
    delete[] ptable;

    //  Closing files
    //
    fclose( outputFile );
    fclose( inputFile );
  }
  
  void Decode( const char *inputFilename, const char *outputFilename )
  {
    //  Opening input file
    //
    FILE *inputFile;
    inputFile = fopen( inputFilename, "r" );
    assert( inputFile );

    //  Loading codes
    //
    fscanf( inputFile, "%i", &tsize );
    char ch, code[128];
    float p;
    int i;
    fgetc( inputFile ); // skip end line
    for( i=0; i<tsize; i++ )
    {
      ch = fgetc(inputFile);
      fscanf( inputFile, "%f %s", &p, code );
      codes[ch] = code;
      fgetc(inputFile); // skip end line
    }
    fgetc(inputFile); // skip end line

    //  Opening output file
    //
    FILE *outputFile;
    outputFile = fopen( outputFilename, "w" );
    assert( outputFile );

    //  Decoding and outputing to file
    //
    string accum = "";
    map<char, string>::iterator ci;
    while((ch = fgetc(inputFile)) != EOF)
    {
      accum += ch;
      for( ci=codes.begin(); ci!=codes.end(); ++ci )
        if( !strcmp( (*ci).second.c_str(), accum.c_str() ) )
        {
          accum = "";
          printf( "%c", (*ci).first );
          fprintf( outputFile, "%c", (*ci).first );
        }
    }
    printf(NL);

    //  Cleaning
    //
    fclose( outputFile );
    fclose( inputFile );
  }

private:
  void EncHuffman()
  {
    //  Creating leaves (initial top-nodes)
    //
    treenode *n;
    vector<treenode*> tops; // top-nodes
    int i, numtop=tsize;
    for( i=0; i<numtop; i++ )
    {
      n = new treenode;
      assert( n );
      n->ch = ptable[i].ch;
      n->p = ptable[i].p;
      n->left = NULL;
      n->right = NULL;
      tops.push_back( n );
    }

    //  Building binary tree.
    //  Combining last two nodes, replacing them by new node
    //  without invalidating sort
    //
    while( numtop > 1 )
    {
      n = new treenode;
      assert( n );
      n->p = tops[numtop-2]->p + tops[numtop-1]->p;
      n->left = tops[numtop-2];
      n->right = tops[numtop-1];
      if( n->left->p < n->right->p )
      {
        n->lcode = '0';
        n->rcode = '1';
      }
      else
      {
        n->lcode = '1';
        n->rcode = '0';
      }
      tops.pop_back();
      tops.pop_back();
      bool isins = false;
      std::vector<treenode*>::iterator ti;
      for( ti=tops.begin(); ti!=tops.end(); ++ti )
        if( (*ti)->p < n->p )
        {
          tops.insert( ti, n );
          isins = true;
          break;
        }
      if( !isins ) tops.push_back( n );
      numtop--;
    }

    //  Building codes
    //
    treenode *root = tops[0];
    GenerateCode( root );

    //  Cleaning
    //
    DestroyNode( root );
    tops.clear();
  }
  
  void GenerateCode( treenode *node ) // for outside call: node is root
  {
    static string sequence = "";
    if( node->left )
    {
      sequence += node->lcode;
      GenerateCode( node->left );
    }

    if( node->right )
    {
      sequence += node->rcode;
      GenerateCode( node->right );
    }

    if( !node->left && !node->right )
      codes[node->ch] = sequence;

    int l = (int)sequence.length();
    if( l > 1 ) sequence = sequence.substr( 0, l-1 );
    else sequence = "";
  }
  
  void DestroyNode( treenode *node ) // for outside call: node is root
  {
    if( node->left )
    {
      DestroyNode( node->left );
      delete node->left;
      node->left = NULL;
    }

    if( node->right )
    {
      DestroyNode( node->right );
      delete node->right;
      node->right = NULL;
    }
  }
};

int show_usage() {
  printf("Huffman coding algorithm"NL);
  printf("by Sergey Tikhonov (st@haqu.net)"NL);
  printf(NL);
  printf("Usage: huffman [OPTIONS] input [output]"NL);
  printf("  The default action is to encode input file."NL);
  printf("  -d\tDecode file."NL);
  printf(NL);
  printf("Examples:"NL);
  printf("  huffman input.txt"NL);
  printf("  huffman input.txt encoded.txt"NL);
  printf("  huffman -d encoded.txt"NL);
  printf(NL);
  exit(0);
}

int main(int argc, char **argv)
{
  int i = 1;
  int dFlag = 0;
  char inputFilename[128];
  char outputFilename[128];

  printf(NL);

  if(i == argc) show_usage();

  if(strcmp(argv[i],"-d") == 0) {
    dFlag = 1;
    i++;
    if(i == argc) show_usage();
  }

  strcpy(inputFilename, argv[i]);
  i++;

  if(i < argc) {
    strcpy(outputFilename, argv[i]);
  } else {
    if(dFlag) {
      strcpy(outputFilename, "decoded.txt");
    } else {
      strcpy(outputFilename, "encoded.txt");
    }
  }

  //  Calling encoding or decoding subroutine
  //
  Coder *coder;
  coder = new Coder;
  assert(coder);
  if(!dFlag) {
    coder->Encode( inputFilename, outputFilename );
  } else {
    coder->Decode( inputFilename, outputFilename );
  }
  delete coder;

  printf(NL);

  return 0;
}
