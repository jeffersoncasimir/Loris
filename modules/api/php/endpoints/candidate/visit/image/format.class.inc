<?php declare(strict_types=1);

/**
 * PHP Version 7
 *
 * @category API
 * @package  Loris
 * @author   Xavier Lecours Boucher <xavier.lecours@mcin.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris
 */
namespace LORIS\api\Endpoints\Candidate\Visit\Image;

use \Psr\Http\Message\ServerRequestInterface;
use \Psr\Http\Message\ResponseInterface;
use \LORIS\api\Endpoint;

/**
 * This class handles request to an image alternate formats.
 *
 * @category API
 * @package  Loris
 * @author   Xavier Lecours Boucher <xavier.lecours@mcin.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris
 */
class Format extends Endpoint
{
    /**
     * The requested Image
     *
     * @var \LORIS\Image
     */
    private $_image;

    /**
     * Contructor
     *
     * @param \LORIS\Image $image The requested image
     */
    public function __construct(\LORIS\Image $image)
    {
        $this->_image = $image;
    }

    /**
     * Return which methods are supported by this endpoint.
     *
     * @return array supported HTTP methods
     */
    protected function allowedMethods() : array
    {
        return ['GET'];
    }

    /**
     * Versions of the LORIS API which are supported by this
     * endpoint.
     *
     * @return array a list of supported API versions.
     */
    protected function supportedVersions() : array
    {
        return [
            'v0.0.3',
            'v0.0.4-dev',
        ];
    }

    /**
     * Delegates the request to a specialized endpoint according to specified
     * the format.
     *
     * @param ServerRequestInterface $request The incoming PSR7 request
     *
     * @return ResponseInterface The outgoing PSR7 response
     */
    public function handle(ServerRequestInterface $request) : ResponseInterface
    {
        $pathparts = $request->getAttribute('pathparts');
        if (count($pathparts) !== 1) {
            return new \LORIS\Http\Response\JSON\NotFound();
        }

        $format = array_shift($pathparts);
        switch ($format) {
        case 'raw':
            $handler = new Format\Raw($this->_image);
            break;
        case 'brainbrowser':
            $handler = new Format\Brainbrowser($this->_image);
            break;
        case 'thumbnail':
        case 'nifti':
        case 'bval':
        case 'bvec':
        case 'bidsjson':
            $handler = new Format\DownloadFile($this->_image, $format);
            break;
        default:
            return new \LORIS\Http\Response\JSON\UnsupportedMediaType();
        }

        $newrequest = $request
            ->withAttribute('pathparts', $pathparts);

        return $handler->process(
            $newrequest,
            $handler
        );
    }
}
